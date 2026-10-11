// Audio: capture-card audio passthrough to any output device, microphone
// capture, and taps that feed the recorder. Backed by miniaudio
// (PulseAudio/PipeWire/ALSA on Linux, WASAPI on Windows).
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct AudioDeviceInfo {
  std::string id;    // unique display name (" (2)" appended on duplicates)
  std::string name;  // raw backend name
  bool isDefault = false;
};

// Single-producer / single-consumer float ring buffer (interleaved frames).
class SampleRing {
 public:
  explicit SampleRing(size_t capacitySamples = 48000 * 2 * 4);
  void write(const float* data, size_t samples);
  size_t read(float* dst, size_t samples);
  size_t available() const;
  void discard(size_t samples);
  void clear();

 private:
  std::vector<float> buf_;
  std::atomic<size_t> w_{0}, r_{0};
};

class AudioEngine {
 public:
  static constexpr int kRate = 48000;
  static constexpr int kChannels = 2;

  AudioEngine();
  ~AudioEngine();
  bool init(std::string& err);
  void shutdown();

  // Re-enumerates devices. Safe to call from a worker thread.
  void refreshDevices();
  std::vector<AudioDeviceInfo> inputs() const;
  std::vector<AudioDeviceInfo> outputs() const;
  std::string backendName() const;

  // Game audio: capture `inId` → play on `outId` ("" = system default).
  bool startPassthrough(const std::string& inId, const std::string& outId, std::string& err);
  // Quest: game audio already captured elsewhere (the capture card's USB
  // audio, read by the app; `channels` 2 = stereo, 6 = 5.1) → play on
  // `outId`, with the same volume / taps. On the headset this is a plain
  // AAudio stream with the card's own channel count, so the system places
  // 5.1 around the listener; recordings get a stereo downmix.
  bool startPassthroughFrom(std::shared_ptr<SampleRing> src, int channels, const std::string& outId,
                            std::string& err);
  // The output stream died (device unplugged / rerouted): restart it.
  bool passthroughLost() const { return extLost_.load(); }
  void stopPassthrough();
  bool passthroughRunning() const;
  void setVolume(float v) { volume_ = v; }

  // Microphone (only goes to recordings, never to the speakers).
  bool startMic(const std::string& id, std::string& err);
  void stopMic();
  bool micRunning() const;

  // Recording taps.
  void setTap(bool on);
  SampleRing& gameRing() { return game_; }
  SampleRing& micRing() { return mic_; }

  // Called from the audio threads.
  void onDuplex(float* out, const float* in, uint32_t frames);
  void onMic(const float* in, uint32_t frames);
  void onPlay(float* out, uint32_t frames);
  void onPlayLost() { extLost_ = true; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  mutable std::mutex mu_;
  std::vector<AudioDeviceInfo> inputs_, outputs_;
  std::atomic<float> volume_{1.f};
  std::atomic<bool> tap_{false};
  SampleRing game_, mic_;
  std::shared_ptr<SampleRing> ext_;
  int extCh_ = 2;
  bool extPrimed_ = false;
  std::atomic<bool> extLost_{false};
  std::vector<float> tapMix_;  // 5.1 → stereo for the recording tap
};
