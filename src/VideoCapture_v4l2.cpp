// Linux Video4Linux2 capture backend. Works with any UVC / V4L2 capture card.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

#include "DeviceNames.h"
#include "FrameDecoder.h"
#include "VideoCapture.h"

namespace {

int xioctl(int fd, unsigned long req, void* arg) {
  int r;
  do {
    r = ioctl(fd, req, arg);
  } while (r == -1 && errno == EINTR);
  return r;
}

const int kStdSizes[][2] = {{3840, 2160}, {2560, 1440}, {1920, 1080}, {1600, 900}, {1280, 720},
                            {1024, 768},  {800, 600},   {720, 480},   {640, 480},  {640, 360}};
const double kStdFps[] = {240, 144, 120, 100, 60, 50, 30, 25, 24, 15};

std::vector<double> enumFps(int fd, uint32_t pixfmt, int w, int h) {
  std::vector<double> out;
  v4l2_frmivalenum iv{};
  iv.pixel_format = pixfmt;
  iv.width = w;
  iv.height = h;
  for (iv.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &iv) == 0; ++iv.index) {
    if (iv.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
      if (iv.discrete.numerator)
        out.push_back((double)iv.discrete.denominator / iv.discrete.numerator);
    } else {
      double maxF = iv.stepwise.min.numerator ? (double)iv.stepwise.min.denominator / iv.stepwise.min.numerator : 60;
      double minF = iv.stepwise.max.numerator ? (double)iv.stepwise.max.denominator / iv.stepwise.max.numerator : 1;
      for (double f : kStdFps)
        if (f <= maxF + 0.01 && f >= minF - 0.01) out.push_back(f);
      break;
    }
  }
  if (out.empty()) out.push_back(30);
  std::sort(out.rbegin(), out.rend());
  // Dedup (e.g. 60 and 59.94 both present → keep both; values differ).
  out.erase(std::unique(out.begin(), out.end(), [](double a, double b) { return std::fabs(a - b) < 0.001; }),
            out.end());
  return out;
}

bool probeDevice(const std::string& path, VideoDeviceInfo& info, std::string& busInfo) {
  int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0) return false;
  v4l2_capability cap{};
  if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
    ::close(fd);
    return false;
  }
  uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
  if (!(caps & V4L2_CAP_VIDEO_CAPTURE) || !(caps & V4L2_CAP_STREAMING)) {
    ::close(fd);
    return false;
  }
  info.path = path;
  info.name = reinterpret_cast<const char*>(cap.card);
  info.driver = reinterpret_cast<const char*>(cap.driver);
  // Many UVC devices report "Name: Name" — tidy that up.
  auto colon = info.name.find(": ");
  if (colon != std::string::npos && info.name.substr(0, colon) == info.name.substr(colon + 2))
    info.name = info.name.substr(0, colon);
  busInfo = reinterpret_cast<const char*>(cap.bus_info);
  info.id = busInfo + "|" + info.name;

  v4l2_fmtdesc fd_{};
  fd_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  for (fd_.index = 0; xioctl(fd, VIDIOC_ENUM_FMT, &fd_) == 0; ++fd_.index) {
    uint32_t pf = fd_.pixelformat;
    if (!fourccSupported(pf)) continue;
    v4l2_frmsizeenum fs{};
    fs.pixel_format = pf;
    for (fs.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fs) == 0; ++fs.index) {
      if (fs.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
        VideoMode m;
        m.width = fs.discrete.width;
        m.height = fs.discrete.height;
        m.fourcc = pf;
        m.fps = enumFps(fd, pf, m.width, m.height);
        info.modes.push_back(m);
      } else {
        for (auto& s : kStdSizes) {
          if ((uint32_t)s[0] < fs.stepwise.min_width || (uint32_t)s[0] > fs.stepwise.max_width) continue;
          if ((uint32_t)s[1] < fs.stepwise.min_height || (uint32_t)s[1] > fs.stepwise.max_height) continue;
          VideoMode m;
          m.width = s[0];
          m.height = s[1];
          m.fourcc = pf;
          m.fps = enumFps(fd, pf, m.width, m.height);
          info.modes.push_back(m);
        }
        break;
      }
    }
  }
  ::close(fd);
  return !info.modes.empty();
}

PixFmt pixFor(uint32_t f) {
  switch (f) {
    case FCC_YUYV: return PixFmt::YUYV;
    case FCC_UYVY: return PixFmt::UYVY;
    case FCC_NV12: return PixFmt::NV12;
    case FCC_NV21: return PixFmt::NV21;
    case FCC_I420:
    case FCC_YV12: return PixFmt::I420;
    case FCC_RGB3: return PixFmt::RGB24;
    case FCC_BGR3: return PixFmt::BGR24;
    case FCC_BGRA:
    case FCC_BGRX: return PixFmt::BGRA;
    case FCC_GREY: return PixFmt::GRAY8;
    default: return PixFmt::Unknown;
  }
}

}  // namespace

std::vector<VideoDeviceInfo> enumerateVideoDevices() {
  std::vector<VideoDeviceInfo> out;
  std::vector<std::pair<int, std::string>> nodes;
  if (DIR* d = opendir("/dev")) {
    while (dirent* e = readdir(d)) {
      if (strncmp(e->d_name, "video", 5) == 0) {
        char* end = nullptr;
        long n = strtol(e->d_name + 5, &end, 10);
        if (end && *end == 0) nodes.push_back({(int)n, std::string("/dev/") + e->d_name});
      }
    }
    closedir(d);
  }
  std::sort(nodes.begin(), nodes.end());
  if (testPatternEnabled()) {
    out.push_back(testPatternDevice(0));
    out.push_back(testPatternDevice(1));
  }
  for (auto& n : nodes) {
    VideoDeviceInfo info;
    std::string bus;
    if (probeDevice(n.second, info, bus)) {
      out.push_back(std::move(info));
    }
  }
  DuplicateNamer namer;
  for (auto& d : out) d.name = namer(d.name);
  return out;
}

std::string videoDeviceSignature() {
  std::string sig;
  std::vector<std::string> names;
  if (DIR* d = opendir("/dev")) {
    while (dirent* e = readdir(d))
      if (strncmp(e->d_name, "video", 5) == 0) names.push_back(e->d_name);
    closedir(d);
  }
  std::sort(names.begin(), names.end());
  for (auto& n : names) {
    struct stat st;
    std::string p = "/dev/" + n;
    if (stat(p.c_str(), &st) == 0) sig += n + ":" + std::to_string((long long)st.st_ctime) + ":" + std::to_string((unsigned long long)st.st_rdev) + ";";
  }
  return sig;
}

struct VideoCapture::Impl {
  int fd = -1;
  struct Buf {
    void* ptr = nullptr;
    size_t len = 0;
  };
  std::vector<Buf> bufs;
  std::thread th;
  std::atomic<bool> stop{false};
  uint32_t fourcc = 0;
  int w = 0, h = 0, bpl = 0;
  DecodePool dec;
  bool compressed = false;
  bool yv12 = false;
  bool bt709 = true;
  bool full = false;
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
  Impl& I = *impl_;
  I.fd = ::open(dev.path.c_str(), O_RDWR | O_NONBLOCK);
  if (I.fd < 0) {
    err = std::string("cannot open ") + dev.path + ": " + strerror(errno);
    if (errno == EACCES) err += " (add your user to the 'video' group)";
    return false;
  }
  v4l2_format fmt{};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = want.width;
  fmt.fmt.pix.height = want.height;
  fmt.fmt.pix.pixelformat = want.fourcc ? want.fourcc : FCC_MJPG;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  if (xioctl(I.fd, VIDIOC_S_FMT, &fmt) < 0) {
    err = std::string("VIDIOC_S_FMT failed: ") + strerror(errno);
    if (errno == EBUSY) err = "Device is busy (in use by another application)";
    ::close(I.fd);
    I.fd = -1;
    return false;
  }
  I.w = fmt.fmt.pix.width;
  I.h = fmt.fmt.pix.height;
  I.bpl = fmt.fmt.pix.bytesperline;
  I.fourcc = fmt.fmt.pix.pixelformat;
  I.compressed = fourccIsCompressed(I.fourcc);
  I.yv12 = I.fourcc == FCC_YV12;
  // Colour: honour what the driver says, else HDMI-capture defaults
  // (limited range; BT.709 for HD, BT.601 for SD).
  I.bt709 = I.h >= 720;
  if (fmt.fmt.pix.ycbcr_enc == V4L2_YCBCR_ENC_601) I.bt709 = false;
  if (fmt.fmt.pix.ycbcr_enc == V4L2_YCBCR_ENC_709) I.bt709 = true;
  I.full = fmt.fmt.pix.quantization == V4L2_QUANTIZATION_FULL_RANGE;

  v4l2_streamparm parm{};
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (want.fps > 0) {
    // Use a 1001-based numerator for NTSC rates like 59.94.
    double f = want.fps;
    if (std::fabs(f * 1001.0 / 1000.0 - std::round(f * 1001.0 / 1000.0)) < 0.01 && std::fabs(f - std::round(f)) > 0.01) {
      parm.parm.capture.timeperframe.numerator = 1001;
      parm.parm.capture.timeperframe.denominator = (uint32_t)std::lround(f * 1001.0);
    } else {
      parm.parm.capture.timeperframe.numerator = 1;
      parm.parm.capture.timeperframe.denominator = (uint32_t)std::lround(f);
    }
    xioctl(I.fd, VIDIOC_S_PARM, &parm);
  }
  double fps = want.fps;
  v4l2_streamparm gp{};
  gp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(I.fd, VIDIOC_G_PARM, &gp) == 0 && gp.parm.capture.timeperframe.numerator)
    fps = (double)gp.parm.capture.timeperframe.denominator / gp.parm.capture.timeperframe.numerator;

  if (I.compressed) {
    Compressed c = Compressed::MJPEG;
    if (I.fourcc == FCC_H264) c = Compressed::H264;
    if (I.fourcc == FCC_HEVC) c = Compressed::HEVC;
    if (!I.dec.open(c, I.w, I.h, [this] { return acquireFrame(); }, [this](FramePtr f) { publish(f); }, err)) {
      ::close(I.fd);
      I.fd = -1;
      return false;
    }
  } else if (pixFor(I.fourcc) == PixFmt::Unknown) {
    err = "unsupported pixel format " + fourccName(I.fourcc);
    ::close(I.fd);
    I.fd = -1;
    return false;
  }

  v4l2_requestbuffers req{};
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (xioctl(I.fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
    err = std::string("VIDIOC_REQBUFS failed: ") + strerror(errno);
    ::close(I.fd);
    I.fd = -1;
    return false;
  }
  I.bufs.resize(req.count);
  for (uint32_t i = 0; i < req.count; ++i) {
    v4l2_buffer b{};
    b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;
    b.index = i;
    if (xioctl(I.fd, VIDIOC_QUERYBUF, &b) < 0) {
      err = "VIDIOC_QUERYBUF failed";
      close();
      return false;
    }
    I.bufs[i].len = b.length;
    I.bufs[i].ptr = mmap(nullptr, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, I.fd, b.m.offset);
    if (I.bufs[i].ptr == MAP_FAILED) {
      I.bufs[i].ptr = nullptr;
      err = "mmap failed";
      close();
      return false;
    }
    xioctl(I.fd, VIDIOC_QBUF, &b);
  }
  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(I.fd, VIDIOC_STREAMON, &type) < 0) {
    err = std::string("VIDIOC_STREAMON failed: ") + strerror(errno);
    if (errno == ENOSPC) err = "Not enough USB bandwidth for this mode — try MJPEG or a lower resolution";
    close();
    return false;
  }
  {
    std::lock_guard<std::mutex> lk(mu_);
    actual_.width = I.w;
    actual_.height = I.h;
    actual_.fps = fps;
    actual_.fourcc = I.fourcc;
    name_ = dev.name;
  }
  I.stop = false;
  running_ = true;
  I.th = std::thread([this] {
    Impl& I = *impl_;
    int consecutiveErrors = 0;
    while (!I.stop) {
      pollfd p{I.fd, POLLIN, 0};
      int r = poll(&p, 1, 200);
      if (r < 0) {
        if (errno == EINTR) continue;
        reportFailure("poll failed");
        break;
      }
      if (r == 0) continue;
      if (p.revents & (POLLERR | POLLHUP)) {
        reportFailure("Capture device disconnected");
        break;
      }
      // Drain: keep only the newest filled buffer to minimise latency.
      v4l2_buffer newest{};
      bool have = false;
      while (true) {
        v4l2_buffer b{};
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        if (xioctl(I.fd, VIDIOC_DQBUF, &b) < 0) {
          if (errno == EAGAIN) break;
          if (errno == ENODEV || errno == EIO) {
            if (++consecutiveErrors > 20 || errno == ENODEV) {
              reportFailure("Capture device disconnected");
              I.stop = true;
            }
          }
          break;
        }
        consecutiveErrors = 0;
        if (have) xioctl(I.fd, VIDIOC_QBUF, &newest);
        newest = b;
        have = true;
      }
      if (!have) continue;
      const uint8_t* data = (const uint8_t*)I.bufs[newest.index].ptr;
      size_t used = newest.bytesused ? newest.bytesused : I.bufs[newest.index].len;
      bool corrupt = (newest.flags & V4L2_BUF_FLAG_ERROR) != 0;
      if (!corrupt) {
        if (I.compressed) {
          I.dec.submit(data, used);
        } else {
          FramePtr f = acquireFrame();
          bool ok;
          ok = fillRawFrame(pixFor(I.fourcc), I.w, I.h, I.bpl, data, used, *f);
          if (ok && I.yv12) std::swap(f->plane[1], f->plane[2]);
          f->bt709 = I.bt709;
          f->fullRange = I.full || pixFor(I.fourcc) == PixFmt::RGB24 || pixFor(I.fourcc) == PixFmt::BGR24 ||
                         pixFor(I.fourcc) == PixFmt::BGRA;
          if (ok) publish(f);
        }
      }
      xioctl(I.fd, VIDIOC_QBUF, &newest);
    }
    running_ = false;
  });
  return true;
}

void VideoCapture::close() {
  closeTestPattern();
  if (!impl_) return;
  Impl& I = *impl_;
  I.stop = true;
  if (I.th.joinable()) I.th.join();
  if (I.fd >= 0) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(I.fd, VIDIOC_STREAMOFF, &type);
  }
  for (auto& b : I.bufs)
    if (b.ptr) munmap(b.ptr, b.len);
  I.bufs.clear();
  if (I.fd >= 0) {
    // Release the buffers so the next S_FMT is allowed.
    v4l2_requestbuffers req{};
    req.count = 0;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(I.fd, VIDIOC_REQBUFS, &req);
    ::close(I.fd);
  }
  I.fd = -1;
  I.dec.close();
  running_ = false;
  std::lock_guard<std::mutex> lk(mu_);
  latest_.reset();
}
