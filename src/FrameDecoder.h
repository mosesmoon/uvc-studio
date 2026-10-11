#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Frame.h"

// Codec types a capture device can deliver compressed.
enum class Compressed { None, MJPEG, H264, HEVC };

// Turns compressed capture payloads (MJPEG / H.264 / HEVC) into planar YUV
// frames: libavcodec on desktop; on Meta Quest libavcodec for MJPEG and the
// headset's built-in hardware decoder (MediaCodec) for H.264 / HEVC. One
// instance per capture stream, used only from its decode thread.
class FrameDecoder {
 public:
  FrameDecoder() = default;
  ~FrameDecoder();
  FrameDecoder(const FrameDecoder&) = delete;
  FrameDecoder& operator=(const FrameDecoder&) = delete;

  bool open(Compressed codec, int width, int height, std::string& err);
  void close();
  // Decodes `data` into `out`. Returns false if no picture was produced
  // (corrupt payload, decoder still buffering, ...).
  bool decode(const uint8_t* data, size_t size, Frame& out);

 private:
  struct Impl;
  Impl* impl_ = nullptr;
};

// Copies a raw (uncompressed) capture buffer into `out.storage` and fills the
// plane pointers. Returns false for unsupported formats / short buffers.
bool fillRawFrame(PixFmt fmt, int width, int height, int bytesPerLine, const uint8_t* data, size_t size,
                  Frame& out);

// Decodes compressed capture payloads off the capture thread. For MJPEG
// (intra-only) several decoders run in parallel and results are published
// newest-wins, so throughput scales with cores without adding queueing
// latency. H.264 / HEVC use a single in-order decoder.
class DecodePool {
 public:
  using Acquire = std::function<FramePtr()>;
  using Publish = std::function<void(FramePtr)>;
  ~DecodePool();
  bool open(Compressed codec, int width, int height, Acquire acquire, Publish publish, std::string& err);
  void submit(const uint8_t* data, size_t size);
  void close();

 private:
  struct Worker {
    FrameDecoder dec;
    std::thread th;
    std::mutex mu;
    std::condition_variable cv;
    std::deque<std::pair<uint64_t, std::vector<uint8_t>>> q;
    bool busy = false;
  };
  void run(Worker* w);
  std::vector<std::unique_ptr<Worker>> workers_;
  std::atomic<bool> stop_{false};
  std::atomic<uint64_t> lastPublished_{0};
  uint64_t seq_ = 0;
  size_t rr_ = 0;
  bool ordered_ = false;
  Acquire acquire_;
  Publish publish_;
  std::mutex pubMu_;
};
