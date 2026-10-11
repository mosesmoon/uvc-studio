#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <set>

#include "FrameDecoder.h"
#include "stb_image.h"
#include "stb_image_write.h"

#include "VideoCapture.h"

bool fourccIsCompressed(uint32_t f) {
  return f == FCC_MJPG || f == FCC_JPEG || f == FCC_H264 || f == FCC_HEVC;
}

bool fourccSupported(uint32_t f) {
  switch (f) {
    case FCC_YUYV:
    case FCC_UYVY:
    case FCC_NV12:
    case FCC_NV21:
    case FCC_I420:
    case FCC_YV12:
    case FCC_MJPG:
    case FCC_JPEG:
    case FCC_H264:
    case FCC_HEVC:
    case FCC_RGB3:
    case FCC_BGR3:
    case FCC_BGRA:
    case FCC_BGRX:
    case FCC_GREY: return true;
    default: return false;
  }
}

std::string fourccName(uint32_t f) {
  std::string s;
  for (int i = 0; i < 4; ++i) {
    char c = (char)((f >> (8 * i)) & 0xff);
    s += (c >= 32 && c < 127) ? c : '?';
  }
  return s;
}

std::string fourccFriendly(uint32_t f) {
  if (f == FCC_MJPG || f == FCC_JPEG) return "MJPEG";
  if (f == FCC_H264) return "H.264";
  if (f == FCC_HEVC) return "HEVC";
  if (f == FCC_YUYV) return "YUY2";
  if (f == FCC_UYVY) return "UYVY";
  if (f == FCC_NV12) return "NV12";
  if (f == FCC_I420) return "I420";
  if (f == FCC_YV12) return "YV12";
  if (f == FCC_RGB3 || f == FCC_BGR3) return "RGB24";
  if (f == FCC_BGRA || f == FCC_BGRX) return "RGB32";
  return fourccName(f);
}

std::vector<std::pair<int, int>> VideoDeviceInfo::resolutions() const {
  std::set<std::pair<int, int>> s;
  for (auto& m : modes)
    if (fourccSupported(m.fourcc)) s.insert({m.width, m.height});
  std::vector<std::pair<int, int>> v(s.begin(), s.end());
  std::sort(v.begin(), v.end(), [](auto& a, auto& b) {
    long pa = (long)a.first * a.second, pb = (long)b.first * b.second;
    if (pa != pb) return pa > pb;
    return a.first > b.first;
  });
  return v;
}

std::vector<int> VideoDeviceInfo::frameRates(int w, int h) const {
  std::set<int> s;
  for (auto& m : modes)
    if (m.width == w && m.height == h && fourccSupported(m.fourcc))
      for (double f : m.fps) s.insert((int)std::lround(f));
  std::vector<int> v(s.rbegin(), s.rend());
  return v;
}

int VideoDeviceInfo::maxWidth() const {
  int m = 0;
  for (auto& x : modes) m = std::max(m, x.width);
  return m;
}
int VideoDeviceInfo::maxHeight() const {
  int m = 0;
  for (auto& x : modes) m = std::max(m, x.height);
  return m;
}
double VideoDeviceInfo::maxFps() const {
  double m = 0;
  for (auto& x : modes)
    for (double f : x.fps) m = std::max(m, f);
  return m;
}

static int formatRank(uint32_t f) {
  // Lower is better. Uncompressed first (lowest latency, no decode cost),
  // then MJPEG, then long-GOP codecs.
  switch (f) {
    case FCC_NV12: return 0;
    case FCC_YUYV: return 1;
    case FCC_UYVY: return 2;
    case FCC_I420: return 3;
    case FCC_YV12: return 3;
    case FCC_NV21: return 3;
    case FCC_BGRX:
    case FCC_BGRA: return 4;
    case FCC_RGB3:
    case FCC_BGR3: return 5;
    case FCC_MJPG: return 10;
    case FCC_JPEG: return 11;
    case FCC_H264: return 20;
    case FCC_HEVC: return 21;
    case FCC_GREY: return 30;
    default: return 100;
  }
}

StreamSettings chooseMode(const VideoDeviceInfo& dev, int w, int h, double fps, uint32_t prefer) {
  StreamSettings out;
  out.width = w;
  out.height = h;
  out.fps = fps;
  out.fourcc = prefer;
  if (dev.modes.empty()) return out;

  // 1) resolution: exact, else the closest by area not exceeding, else closest.
  auto res = dev.resolutions();
  if (res.empty()) return out;
  std::pair<int, int> best = res.front();
  bool exact = false;
  for (auto& r : res)
    if (r.first == w && r.second == h) {
      best = r;
      exact = true;
    }
  if (!exact) {
    long target = (long)w * h;
    long bestScore = -1;
    for (auto& r : res) {
      long a = (long)r.first * r.second;
      long score = a <= target ? (target - a) : (a - target) * 4;  // prefer smaller
      if (bestScore < 0 || score < bestScore) {
        bestScore = score;
        best = r;
      }
    }
  }
  out.width = best.first;
  out.height = best.second;

  // 2) candidate modes at that resolution.
  struct Cand {
    uint32_t fcc;
    double fps;
    double diff;
    int rank;
  };
  std::vector<Cand> cands;
  for (auto& m : dev.modes) {
    if (m.width != out.width || m.height != out.height || !fourccSupported(m.fourcc)) continue;
    for (double f : m.fps) {
      double d = std::fabs(f - fps);
      cands.push_back({m.fourcc, f, d, formatRank(m.fourcc)});
    }
  }
  if (cands.empty()) return out;
  // Closest frame rate wins; among equal rates prefer the requested fourcc,
  // then the format rank.
  double bestDiff = 1e9;
  for (auto& c : cands) bestDiff = std::min(bestDiff, c.diff);
  // Treat 59.94 vs 60 as equal.
  std::vector<Cand> top;
  for (auto& c : cands)
    if (c.diff <= bestDiff + 0.51) top.push_back(c);
  auto matches = [&](uint32_t f) {
    if (prefer == 0) return false;
    if (prefer == PREFER_UNCOMPRESSED) return !fourccIsCompressed(f);
    if (prefer == PREFER_COMPRESSED) return fourccIsCompressed(f);
    return f == prefer;
  };
  std::sort(top.begin(), top.end(), [&](const Cand& a, const Cand& b) {
    bool pa = matches(a.fcc), pb = matches(b.fcc);
    if (pa != pb) return pa;
    if (a.rank != b.rank) return a.rank < b.rank;
    return a.diff < b.diff;
  });
  out.fourcc = top.front().fcc;
  out.fps = top.front().fps;
  return out;
}



FramePtr VideoCapture::latest(uint64_t lastSeq) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!latest_ || latest_->seq == lastSeq) return nullptr;
  return latest_;
}

StreamSettings VideoCapture::actual() const {
  std::lock_guard<std::mutex> lk(mu_);
  return actual_;
}

std::string VideoCapture::failure() const {
  std::lock_guard<std::mutex> lk(mu_);
  return failure_;
}

void VideoCapture::reportFailure(const std::string& msg) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    failure_ = msg;
  }
  failed_ = true;
}

FramePtr VideoCapture::acquireFrame() {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& f : pool_)
    if (f.use_count() == 1) return f;
  auto f = std::make_shared<Frame>();
  if (pool_.size() < 12) pool_.push_back(f);
  return f;
}

void VideoCapture::publish(FramePtr f) {
  double t = nowSeconds();
  f->timestamp = t;
  std::lock_guard<std::mutex> lk(mu_);
  if (statStart_ == 0) statStart_ = t;
  ++statCount_;
  if (t - statStart_ >= 1.0) {
    measuredFps_ = statCount_ / (t - statStart_);
    statCount_ = 0;
    statStart_ = t;
  }
  f->seq = ++seq_;
  lastFrameT_ = t;
  latest_ = std::move(f);
}

// ---------------------------------------------------------------------------
// Display names of the test devices (UVC_STUDIO_TEST_NAME and
// UVC_STUDIO_TEST_WEBCAM_NAME, for screenshots).
static const char* testPatternName(bool webcam = false) {
  const char* n = getenv(webcam ? "UVC_STUDIO_TEST_WEBCAM_NAME" : "UVC_STUDIO_TEST_NAME");
  return n && *n ? n : (webcam ? "Test Webcam" : "Test Pattern");
}

bool testPatternEnabled() {
  const char* e = getenv("UVC_STUDIO_TEST_PATTERN");
  return e && *e && strcmp(e, "0") != 0;
}

VideoDeviceInfo testPatternDevice(int index) {
  VideoDeviceInfo d;
  d.id = index ? "test:webcam" : "test:pattern";
  d.path = d.id;
  d.name = testPatternName(index != 0);
  for (auto r : {std::pair<int, int>{3840, 2160}, {1920, 1080}, {1280, 720}}) {
    VideoMode m;
    m.width = r.first;
    m.height = r.second;
    m.fourcc = FCC_YUYV;
    m.fps = {120, 60, 30};
    d.modes.push_back(m);
  }
  return d;
}

bool VideoCapture::openTestPattern(const StreamSettings& want) {
  failed_ = false;
  {
    std::lock_guard<std::mutex> lk(mu_);
    actual_ = want;
    const char* m = getenv("UVC_STUDIO_TEST_PATTERN");
    actual_.fourcc = (m && !strcmp(m, "mjpeg")) ? FCC_MJPG : FCC_YUYV;
    name_ = testPatternName();
  }
  genStop_ = false;
  running_ = true;
  genThread_ = std::thread([this, want] {
    int W = want.width, H = want.height;
    double fps = want.fps > 0 ? want.fps : 60;
    std::vector<uint8_t> buf((size_t)W * H * 2);
    const char* mode = getenv("UVC_STUDIO_TEST_PATTERN");
    bool mjpeg = mode && !strcmp(mode, "mjpeg");
    std::vector<uint8_t> rgb(mjpeg ? (size_t)W * H * 3 : 0), jpg;
    DecodePool pool;
    std::string perr;
    if (mjpeg) pool.open(Compressed::MJPEG, W, H, [this] { return acquireFrame(); }, [this](FramePtr f) { publish(f); }, perr);
    double t0 = nowSeconds();
    uint64_t n = 0;
    // Optional still image (UVC_STUDIO_TEST_IMAGE=/path.png), scaled to the
    // mode and slowly panned — handy for judging effects on real content.
    std::vector<uint8_t> still;
    if (const char* img = getenv("UVC_STUDIO_TEST_IMAGE")) {
      int iw, ih, ic;
      if (unsigned char* px = stbi_load(img, &iw, &ih, &ic, 3)) {
        still.resize((size_t)W * H * 3);
        for (int y = 0; y < H; ++y)
          for (int x = 0; x < W; ++x) {
            int sx = x * iw / W, sy = y * ih / H;
            memcpy(&still[((size_t)y * W + x) * 3], &px[((size_t)sy * iw + sx) * 3], 3);
          }
        stbi_image_free(px);
      }
    }
    if (!still.empty()) {
      {
        std::lock_guard<std::mutex> lk(mu_);
        actual_.fourcc = FCC_RGB3;
      }
      while (!genStop_) {
        double target = t0 + n / fps;
        double now = nowSeconds();
        if (now < target) {
          std::this_thread::sleep_for(std::chrono::microseconds((int)((target - now) * 1e6)));
          continue;
        }
        FramePtr f = acquireFrame();
        if (fillRawFrame(PixFmt::RGB24, W, H, W * 3, still.data(), still.size(), *f)) publish(f);
        ++n;
      }
      return;
    }
    // 75% colour bars in YUV (BT.709 limited).
    const uint8_t bars[8][3] = {{180, 128, 128}, {168, 44, 136}, {145, 147, 44}, {133, 63, 52},
                                {63, 193, 204},  {51, 109, 212}, {28, 212, 120}, {16, 128, 128}};
    while (!genStop_) {
      double target = t0 + n / fps;
      double now = nowSeconds();
      if (now < target) {
        std::this_thread::sleep_for(std::chrono::microseconds((int)((target - now) * 1e6)));
        continue;
      }
      int shift = (int)(n * 4) % W;
      for (int y = 0; y < H; ++y) {
        uint8_t* row = buf.data() + (size_t)y * W * 2;
        for (int x = 0; x < W; x += 2) {
          int bx = ((x + shift) % W) * 8 / W;
          const uint8_t* c = bars[bx];
          uint8_t Y = c[0];
          if (y > H * 3 / 4) Y = (uint8_t)(16 + (x * 219) / W);  // luma ramp
          row[x * 2 + 0] = Y;
          row[x * 2 + 1] = y > H * 3 / 4 ? 128 : c[1];
          row[x * 2 + 2] = Y;
          row[x * 2 + 3] = y > H * 3 / 4 ? 128 : c[2];
        }
      }
      // Moving white box.
      int bs = H / 6;
      int bx0 = (int)((n * 7) % (W - bs)), by0 = H / 3;
      for (int y = by0; y < by0 + bs; ++y)
        for (int x = bx0 & ~1; x < bx0 + bs; ++x) buf[(size_t)y * W * 2 + x * 2] = 235;
      if (mjpeg) {
        // Exercise the MJPEG path: YUYV → RGB → JPEG → DecodePool.
        for (int i = 0, j = 0; i < W * H; i += 2, j += 6) {
          const uint8_t* q = &buf[(size_t)i * 2];
          for (int k = 0; k < 2; ++k) {
            float Y = (q[k * 2] - 16) * 1.164f, U = q[1] - 128.f, V = q[3] - 128.f;
            auto cl = [](float v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); };
            rgb[j + k * 3 + 0] = cl(Y + 1.793f * V);
            rgb[j + k * 3 + 1] = cl(Y - 0.213f * U - 0.533f * V);
            rgb[j + k * 3 + 2] = cl(Y + 2.112f * U);
          }
        }
        jpg.clear();
        stbi_write_jpg_to_func([](void* ctx, void* data, int size) {
          auto* v = static_cast<std::vector<uint8_t>*>(ctx);
          v->insert(v->end(), (uint8_t*)data, (uint8_t*)data + size);
        }, &jpg, W, H, 3, rgb.data(), 85);
        pool.submit(jpg.data(), jpg.size());
      } else {
        FramePtr f = acquireFrame();
        if (fillRawFrame(PixFmt::YUYV, W, H, W * 2, buf.data(), buf.size(), *f)) {
          f->bt709 = true;
          f->fullRange = false;
          publish(f);
        }
      }
      ++n;
    }
    running_ = false;
  });
  return true;
}

void VideoCapture::closeTestPattern() {
  genStop_ = true;
  if (genThread_.joinable()) genThread_.join();
}

void requestVideoDevicePermissions() {}
