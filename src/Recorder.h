// MP4 recorder (H.264 + AAC) — what you see is what you record.
// Hardware encoders only (NVENC, VAAPI, AMF, Media Foundation; on Windows
// also Windows' built-in H.264 encoder). No software encoder is bundled.
#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class AudioEngine;

class Recorder {
 public:
  struct Options {
    std::string path;
    int width = 1920;
    int height = 1080;
    int fps = 60;
    int64_t videoBitrate = 20000000;  // 20 Mbps
    bool gameAudio = true;
    bool mic = false;
  };

  Recorder();
  ~Recorder();
  bool start(const Options& o, AudioEngine* audio, std::string& err);
  // RGBA, top-down, tightly packed. `t` = monotonic seconds.
  void pushVideo(std::vector<uint8_t>&& rgba, int w, int h, double t);
  // Android: frames go straight from a GL texture into the hardware
  // encoder's input surface (no readback). Must be called on the GL thread.
  bool acceptsTexture() const;
  void pushTexture(unsigned tex, int w, int h, double t);
  // Requests stop; finalisation happens on the recorder thread.
  void stop();
  bool running() const { return running_.load(); }
  bool busy() const { return thread_.joinable(); }
  void setMic(bool on) { opts_mic_ = on; }
  void setGameAudio(bool on) { opts_game_ = on; }
  std::string encoderName() const;
  // Returns true once after a recording has been finalised.
  bool pollFinished(std::string& path, bool& ok, std::string& err);
  // Target frame interval for pacing the compositor.
  int fps() const { return opts_.fps; }

  struct Impl;  // implementation detail

 private:
  void run();
  Impl* impl_ = nullptr;
  Options opts_;
  AudioEngine* audio_ = nullptr;
  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stopReq_{false};
  std::atomic<bool> opts_mic_{false};
  std::atomic<bool> opts_game_{true};
  mutable std::mutex mu_;
  std::condition_variable cv_;
  struct VFrame {
    std::vector<uint8_t> data;
    int w, h;
    double t;
  };
  std::deque<VFrame> queue_;
  std::string encName_;
  bool finished_ = false;
  bool finOk_ = false;
  std::string finErr_;
  double startT_ = 0;
};
