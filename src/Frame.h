// UVC Studio (native) — a decoded / raw video frame handed from a capture
// thread to the renderer.
#pragma once
#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
struct AVFrame;
}

enum class PixFmt {
  Unknown,
  YUYV,     // packed 4:2:2 Y0 U Y1 V
  UYVY,     // packed 4:2:2 U Y0 V Y1
  NV12,     // Y plane + interleaved UV (4:2:0)
  NV21,     // Y plane + interleaved VU (4:2:0)
  I420,     // planar 4:2:0 (Y, U, V)
  YUV422P,  // planar 4:2:2
  YUV444P,  // planar 4:4:4
  RGB24,
  BGR24,
  RGBA,
  BGRA,
  GRAY8,
};

struct Frame {
  int width = 0;
  int height = 0;
  PixFmt fmt = PixFmt::Unknown;
  // Colour description for YUV formats.
  bool bt709 = true;
  bool fullRange = false;
  // Plane pointers + strides (bytes). Point either into `storage` or into
  // `av` (a referenced libavcodec frame).
  const uint8_t* plane[3] = {nullptr, nullptr, nullptr};
  int stride[3] = {0, 0, 0};
  std::vector<uint8_t> storage;
  AVFrame* av = nullptr;
  uint64_t seq = 0;
  double timestamp = 0.0;  // seconds, monotonic clock

  Frame() = default;
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;
  ~Frame();
  void releaseAv();
};

using FramePtr = std::shared_ptr<Frame>;

// Monotonic seconds.
double nowSeconds();
