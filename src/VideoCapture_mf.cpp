// Windows Media Foundation capture backend. Works with any UVC / AVStream
// capture card.
// Native media types are used (no MF converters) so we get the card's raw
// YUY2/NV12/MJPEG stream with the lowest possible latency; decoding and
// colour conversion happen in DecodePool / on the GPU like on Linux.
#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>

#include "DeviceNames.h"
#include "FrameDecoder.h"
#include "VideoCapture.h"

namespace {

template <class T>
void release(T*& p) {
  if (p) {
    p->Release();
    p = nullptr;
  }
}

std::string narrow(const wchar_t* w) {
  if (!w) return "";
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
  std::string s(n > 0 ? n - 1 : 0, '\0');
  if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
  return s;
}

std::wstring widen(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring w(n > 0 ? n - 1 : 0, L'\0');
  if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
  return w;
}

struct MFInit {
  MFInit() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_LITE);
  }
};
void ensureMF() {
  static MFInit init;
  // COM must be initialised on every thread that touches MF.
  thread_local bool com = [] {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    return true;
  }();
  (void)com;
}

uint32_t fourccFromSubtype(const GUID& g) {
  if (g == MFVideoFormat_YUY2) return FCC_YUYV;
  if (g == MFVideoFormat_UYVY) return FCC_UYVY;
  if (g == MFVideoFormat_NV12) return FCC_NV12;
  if (g == MFVideoFormat_I420 || g == MFVideoFormat_IYUV) return FCC_I420;
  if (g == MFVideoFormat_YV12) return FCC_YV12;
  if (g == MFVideoFormat_MJPG) return FCC_MJPG;
  if (g == MFVideoFormat_H264) return FCC_H264;
  if (g == MFVideoFormat_HEVC) return FCC_HEVC;
  if (g == MFVideoFormat_RGB24) return FCC_BGR3;
  if (g == MFVideoFormat_RGB32 || g == MFVideoFormat_ARGB32) return FCC_BGRX;
  return 0;
}

PixFmt pixFor(uint32_t f) {
  switch (f) {
    case FCC_YUYV: return PixFmt::YUYV;
    case FCC_UYVY: return PixFmt::UYVY;
    case FCC_NV12: return PixFmt::NV12;
    case FCC_I420:
    case FCC_YV12: return PixFmt::I420;
    case FCC_BGR3: return PixFmt::BGR24;
    case FCC_BGRX: return PixFmt::BGRA;
    default: return PixFmt::Unknown;
  }
}

// Cache of modes per symbolic link (activating a device that is already
// streaming can fail, so we keep what we learned earlier).
std::mutex gCacheMu;
std::map<std::string, std::vector<VideoMode>> gModeCache;

bool probeModes(IMFActivate* act, std::vector<VideoMode>& out) {
  IMFMediaSource* src = nullptr;
  if (FAILED(act->ActivateObject(IID_PPV_ARGS(&src)))) return false;
  IMFAttributes* ra = nullptr;
  MFCreateAttributes(&ra, 2);
  ra->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
  IMFSourceReader* rd = nullptr;
  HRESULT hr = MFCreateSourceReaderFromMediaSource(src, ra, &rd);
  release(ra);
  if (SUCCEEDED(hr)) {
    std::map<std::tuple<int, int, uint32_t>, std::vector<double>> acc;
    for (DWORD i = 0;; ++i) {
      IMFMediaType* t = nullptr;
      if (FAILED(rd->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &t))) break;
      GUID sub{};
      t->GetGUID(MF_MT_SUBTYPE, &sub);
      UINT32 w = 0, h = 0, num = 0, den = 0;
      MFGetAttributeSize(t, MF_MT_FRAME_SIZE, &w, &h);
      MFGetAttributeRatio(t, MF_MT_FRAME_RATE, &num, &den);
      uint32_t f = fourccFromSubtype(sub);
      if (f && w && h && den) acc[{(int)w, (int)h, f}].push_back((double)num / den);
      release(t);
    }
    for (auto& [k, fps] : acc) {
      VideoMode m;
      m.width = std::get<0>(k);
      m.height = std::get<1>(k);
      m.fourcc = std::get<2>(k);
      std::sort(fps.rbegin(), fps.rend());
      fps.erase(std::unique(fps.begin(), fps.end(), [](double a, double b) { return std::fabs(a - b) < 0.001; }),
                fps.end());
      m.fps = fps;
      out.push_back(m);
    }
    release(rd);
  }
  src->Shutdown();
  release(src);
  act->ShutdownObject();
  return !out.empty();
}

}  // namespace

std::string videoDeviceSignature() {
  ensureMF();
  IMFAttributes* a = nullptr;
  MFCreateAttributes(&a, 1);
  a->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
  IMFActivate** devs = nullptr;
  UINT32 n = 0;
  std::string sig;
  if (SUCCEEDED(MFEnumDeviceSources(a, &devs, &n))) {
    for (UINT32 i = 0; i < n; ++i) {
      WCHAR* link = nullptr;
      UINT32 len = 0;
      if (SUCCEEDED(devs[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &link, &len))) {
        sig += narrow(link) + ";";
        CoTaskMemFree(link);
      }
      devs[i]->Release();
    }
    CoTaskMemFree(devs);
  }
  release(a);
  return sig;
}

std::vector<VideoDeviceInfo> enumerateVideoDevices() {
  ensureMF();
  std::vector<VideoDeviceInfo> out;
  if (testPatternEnabled()) {
    out.push_back(testPatternDevice(0));
    out.push_back(testPatternDevice(1));
  }
  IMFAttributes* a = nullptr;
  MFCreateAttributes(&a, 1);
  a->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
  IMFActivate** devs = nullptr;
  UINT32 n = 0;
  if (FAILED(MFEnumDeviceSources(a, &devs, &n))) {
    release(a);
    return out;
  }
  std::vector<VideoDeviceInfo> found;
  for (UINT32 i = 0; i < n; ++i) {
    VideoDeviceInfo d;
    WCHAR* s = nullptr;
    UINT32 len = 0;
    if (SUCCEEDED(devs[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &s, &len))) {
      d.name = narrow(s);
      CoTaskMemFree(s);
    }
    if (SUCCEEDED(devs[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &s, &len))) {
      d.path = narrow(s);
      CoTaskMemFree(s);
    }
    d.id = d.path;
    std::vector<VideoMode> modes;
    if (probeModes(devs[i], modes)) {
      std::lock_guard<std::mutex> lk(gCacheMu);
      gModeCache[d.path] = modes;
    } else {
      std::lock_guard<std::mutex> lk(gCacheMu);
      auto it = gModeCache.find(d.path);
      if (it != gModeCache.end()) modes = it->second;
    }
    d.modes = std::move(modes);
    found.push_back(std::move(d));
    devs[i]->Release();
  }
  CoTaskMemFree(devs);
  release(a);
  DuplicateNamer namer;
  for (auto& d : found) {
    d.name = namer(d.name);
    out.push_back(std::move(d));
  }
  return out;
}

class ReaderCallback;

struct VideoCapture::Impl {
  ReaderCallback* cb = nullptr;
  HANDLE flushed = nullptr;
  std::atomic<bool> stop{false};
  int errors = 0;
  IMFSourceReader* reader = nullptr;
  IMFMediaSource* source = nullptr;
  uint32_t fourcc = 0;
  int w = 0, h = 0;
  LONG stride = 0;
  DecodePool dec;
  bool compressed = false;
  bool bt709 = true;
  bool full = false;
  bool bottomUp = false;
  std::vector<uint8_t> flip;
};

static void processSample(VideoCapture* vc, VideoCapture::Impl& I, IMFSample* sample);

class ReaderCallback : public IMFSourceReaderCallback {
 public:
  ReaderCallback(VideoCapture* vc, VideoCapture::Impl* impl) : vc_(vc), I_(impl) {}
  STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
    static const QITAB qit[] = {QITABENT(ReaderCallback, IMFSourceReaderCallback), {0}};
    return QISearch(this, qit, iid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
  STDMETHODIMP_(ULONG) Release() override {
    ULONG r = InterlockedDecrement(&ref_);
    if (r == 0) delete this;
    return r;
  }
  STDMETHODIMP OnReadSample(HRESULT hr, DWORD, DWORD flags, LONGLONG, IMFSample* sample) override {
    VideoCapture::Impl& I = *I_;
    if (I.stop) return S_OK;
    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ERROR)) {
      if (++I.errors > 30 || (flags & MF_SOURCE_READERF_ERROR) || hr == MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED) {
        vc_->reportFailure("Capture device disconnected");
        return S_OK;
      }
    } else if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
      vc_->reportFailure("Capture stream ended");
      return S_OK;
    } else if (sample) {
      I.errors = 0;
      processSample(vc_, I, sample);
    }
    if (!I.stop && I.reader) I.reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
    return S_OK;
  }
  STDMETHODIMP OnFlush(DWORD) override {
    if (I_->flushed) SetEvent(I_->flushed);
    return S_OK;
  }
  STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*) override { return S_OK; }

 private:
  virtual ~ReaderCallback() = default;
  long ref_ = 1;
  VideoCapture* vc_;
  VideoCapture::Impl* I_;
};

VideoCapture::VideoCapture() : impl_(new Impl) {}
std::shared_ptr<SampleRing> VideoCapture::deviceAudio() const { return nullptr; }
int VideoCapture::deviceAudioChannels() const { return 2; }
std::string VideoCapture::deviceAudioInfo() const { return {}; }
VideoCapture::~VideoCapture() { close(); }

bool VideoCapture::open(const VideoDeviceInfo& dev, const StreamSettings& want, std::string& err) {
  close();
  failed_ = false;
  failure_.clear();
  if (dev.path.rfind("test:", 0) == 0) return openTestPattern(want);
  ensureMF();
  Impl& I = *impl_;

  IMFAttributes* a = nullptr;
  MFCreateAttributes(&a, 2);
  a->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
  std::wstring link = widen(dev.path);
  a->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, link.c_str());
  HRESULT hr = MFCreateDeviceSource(a, &I.source);
  release(a);
  if (FAILED(hr)) {
    err = "cannot open device (in use by another app, or camera access blocked in Windows privacy settings)";
    return false;
  }
  IMFAttributes* ra = nullptr;
  MFCreateAttributes(&ra, 4);
  I.cb = new ReaderCallback(this, &I);
  I.flushed = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  I.errors = 0;
  I.stop = false;
  ra->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, I.cb);
  ra->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
  ra->SetUINT32(MF_LOW_LATENCY, TRUE);
  ra->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);
  hr = MFCreateSourceReaderFromMediaSource(I.source, ra, &I.reader);
  release(ra);
  if (FAILED(hr)) {
    err = "cannot create source reader";
    close();
    return false;
  }
  // Find the native type matching the requested mode.
  IMFMediaType* best = nullptr;
  double bestDiff = 1e9;
  for (DWORD i = 0;; ++i) {
    IMFMediaType* t = nullptr;
    if (FAILED(I.reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &t))) break;
    GUID sub{};
    t->GetGUID(MF_MT_SUBTYPE, &sub);
    UINT32 w = 0, h = 0, num = 0, den = 0;
    MFGetAttributeSize(t, MF_MT_FRAME_SIZE, &w, &h);
    MFGetAttributeRatio(t, MF_MT_FRAME_RATE, &num, &den);
    uint32_t f = fourccFromSubtype(sub);
    double fps = den ? (double)num / den : 0;
    if (f == want.fourcc && (int)w == want.width && (int)h == want.height) {
      double d = std::fabs(fps - want.fps);
      if (d < bestDiff) {
        release(best);
        best = t;
        t = nullptr;
        bestDiff = d;
      }
    }
    release(t);
  }
  if (!best) {
    err = "requested mode not offered by the device";
    close();
    return false;
  }
  hr = I.reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, best);
  if (FAILED(hr)) {
    release(best);
    err = "device rejected the video mode";
    close();
    return false;
  }
  UINT32 w = 0, h = 0, num = 0, den = 0;
  MFGetAttributeSize(best, MF_MT_FRAME_SIZE, &w, &h);
  MFGetAttributeRatio(best, MF_MT_FRAME_RATE, &num, &den);
  GUID sub{};
  best->GetGUID(MF_MT_SUBTYPE, &sub);
  I.w = (int)w;
  I.h = (int)h;
  I.fourcc = fourccFromSubtype(sub);
  I.compressed = fourccIsCompressed(I.fourcc);
  I.stride = (LONG)MFGetAttributeUINT32(best, MF_MT_DEFAULT_STRIDE, 0);
  UINT32 nominal = MFGetAttributeUINT32(best, MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown);
  UINT32 matrix = MFGetAttributeUINT32(best, MF_MT_YUV_MATRIX, MFVideoTransferMatrix_Unknown);
  I.full = nominal == MFNominalRange_0_255;
  I.bt709 = matrix == MFVideoTransferMatrix_BT709 || (matrix == MFVideoTransferMatrix_Unknown && I.h >= 720);
  if (matrix == MFVideoTransferMatrix_BT601) I.bt709 = false;
  release(best);
  if (I.stride == 0) {
    int bpp = (I.fourcc == FCC_YUYV || I.fourcc == FCC_UYVY) ? 2 : I.fourcc == FCC_BGR3 ? 3 : I.fourcc == FCC_BGRX ? 4 : 1;
    I.stride = I.w * bpp;
    // RGB types are bottom-up by default in MF.
    if (I.fourcc == FCC_BGR3 || I.fourcc == FCC_BGRX) I.stride = -I.stride;
  }
  I.bottomUp = I.stride < 0;
  if (I.compressed) {
    Compressed c = I.fourcc == FCC_H264 ? Compressed::H264 : I.fourcc == FCC_HEVC ? Compressed::HEVC : Compressed::MJPEG;
    if (!I.dec.open(c, I.w, I.h, [this] { return acquireFrame(); }, [this](FramePtr f) { publish(f); }, err)) {
      close();
      return false;
    }
  } else if (pixFor(I.fourcc) == PixFmt::Unknown) {
    err = "unsupported pixel format";
    close();
    return false;
  }
  {
    std::lock_guard<std::mutex> lk(mu_);
    actual_.width = I.w;
    actual_.height = I.h;
    actual_.fps = den ? (double)num / den : want.fps;
    actual_.fourcc = I.fourcc;
    name_ = dev.name;
  }
  running_ = true;
  hr = I.reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
  if (FAILED(hr)) {
    err = "could not start streaming";
    close();
    return false;
  }
  return true;
}

static void processSample(VideoCapture* vc, VideoCapture::Impl& I, IMFSample* sample) {
  IMFMediaBuffer* buf = nullptr;
  if (FAILED(sample->ConvertToContiguousBuffer(&buf))) return;
  BYTE* data = nullptr;
  DWORD maxLen = 0, len = 0;
  IMF2DBuffer* b2 = nullptr;
  LONG pitch = 0;
  bool locked2d = false;
  if (!I.compressed && SUCCEEDED(buf->QueryInterface(IID_PPV_ARGS(&b2)))) {
    if (SUCCEEDED(b2->Lock2D(&data, &pitch))) locked2d = true;
  }
  if (!locked2d) {
    if (SUCCEEDED(buf->Lock(&data, &maxLen, &len)))
      pitch = I.stride;
    else
      data = nullptr;
  }
  if (data) {
    if (I.compressed) {
      if (!len) buf->GetCurrentLength(&len);
      I.dec.submit(data, len);
    } else {
      FramePtr f = vc->acquireFrame();
      const uint8_t* src = data;
      int bpl = pitch;
      if (bpl < 0) {
        // Bottom-up RGB: flip into a scratch buffer.
        int apl = -bpl;
        const uint8_t* first = locked2d ? data : data + (size_t)apl * (I.h - 1);
        I.flip.resize((size_t)apl * I.h);
        for (int y = 0; y < I.h; ++y) memcpy(&I.flip[(size_t)y * apl], first + (ptrdiff_t)y * bpl, apl);
        src = I.flip.data();
        bpl = apl;
      }
      bool planar420 = I.fourcc == FCC_NV12 || I.fourcc == FCC_I420 || I.fourcc == FCC_YV12;
      size_t total = planar420 ? (size_t)bpl * I.h * 3 / 2 : (size_t)bpl * I.h;
      if (fillRawFrame(pixFor(I.fourcc), I.w, I.h, bpl, src, total, *f)) {
        if (I.fourcc == FCC_YV12) std::swap(f->plane[1], f->plane[2]);
        f->bt709 = I.bt709;
        f->fullRange = I.full || I.fourcc == FCC_BGR3 || I.fourcc == FCC_BGRX;
        vc->publish(f);
      }
    }
    if (locked2d)
      b2->Unlock2D();
    else
      buf->Unlock();
  }
  release(b2);
  release(buf);
}

void VideoCapture::close() {
  closeTestPattern();
  if (!impl_) return;
  Impl& I = *impl_;
  I.stop = true;
  if (I.reader && I.flushed) {
    if (SUCCEEDED(I.reader->Flush((DWORD)MF_SOURCE_READER_ALL_STREAMS))) WaitForSingleObject(I.flushed, 1000);
  }
  I.dec.close();
  release(I.reader);
  if (I.source) {
    I.source->Shutdown();
    release(I.source);
  }
  if (I.cb) {
    I.cb->Release();
    I.cb = nullptr;
  }
  if (I.flushed) {
    CloseHandle(I.flushed);
    I.flushed = nullptr;
  }
  running_ = false;
  std::lock_guard<std::mutex> lk(mu_);
  latest_.reset();
}
