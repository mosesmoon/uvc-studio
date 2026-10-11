#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#include "DeviceNames.h"
#include "miniaudio.h"

#include <algorithm>
#include <cstring>
#include <map>

#include "AudioEngine.h"

// ---------------------------------------------------------------------------
SampleRing::SampleRing(size_t cap) : buf_(cap) {}

void SampleRing::write(const float* data, size_t n) {
  size_t cap = buf_.size();
  size_t w = w_.load(std::memory_order_relaxed);
  size_t r = r_.load(std::memory_order_acquire);
  size_t free = cap - (w - r);
  if (n > free) n = free;  // drop what doesn't fit (reader is behind)
  for (size_t i = 0; i < n; ++i) buf_[(w + i) % cap] = data[i];
  w_.store(w + n, std::memory_order_release);
}

size_t SampleRing::read(float* dst, size_t n) {
  size_t cap = buf_.size();
  size_t r = r_.load(std::memory_order_relaxed);
  size_t w = w_.load(std::memory_order_acquire);
  size_t avail = w - r;
  if (n > avail) n = avail;
  for (size_t i = 0; i < n; ++i) dst[i] = buf_[(r + i) % cap];
  r_.store(r + n, std::memory_order_release);
  return n;
}

size_t SampleRing::available() const { return w_.load(std::memory_order_acquire) - r_.load(std::memory_order_acquire); }

void SampleRing::discard(size_t n) {
  size_t r = r_.load(std::memory_order_relaxed);
  size_t w = w_.load(std::memory_order_acquire);
  if (n > w - r) n = w - r;
  r_.store(r + n, std::memory_order_release);
}

void SampleRing::clear() { r_.store(w_.load(std::memory_order_acquire), std::memory_order_release); }

// ---------------------------------------------------------------------------
struct AudioEngine::Impl {
  ma_context ctx{};
  bool ctxOk = false;
  std::map<std::string, ma_device_id> inIds, outIds;
  ma_device duplex{};
  bool duplexOk = false;
  ma_device mic{};
  bool micOk = false;
  ma_device play{};
  bool playOk = false;
};

static void playCb(ma_device* d, void* out, const void* in, ma_uint32 frames) {
  (void)in;
  static_cast<AudioEngine*>(d->pUserData)->onPlay((float*)out, frames);
}


static void duplexCb(ma_device* d, void* out, const void* in, ma_uint32 frames) {
  static_cast<AudioEngine*>(d->pUserData)->onDuplex((float*)out, (const float*)in, frames);
}

static void micCb(ma_device* d, void* out, const void* in, ma_uint32 frames) {
  (void)out;
  static_cast<AudioEngine*>(d->pUserData)->onMic((const float*)in, frames);
}

AudioEngine::AudioEngine() : impl_(new Impl) {}
AudioEngine::~AudioEngine() { shutdown(); }

bool AudioEngine::init(std::string& err) {
  ma_context_config cfg = ma_context_config_init();
  cfg.pulse.pApplicationName = "UVC Studio";
  cfg.threadPriority = ma_thread_priority_realtime;
  if (ma_context_init(nullptr, 0, &cfg, &impl_->ctx) != MA_SUCCESS) {
    err = "audio backend unavailable";
    return false;
  }
  impl_->ctxOk = true;
  refreshDevices();
  return true;
}

void AudioEngine::shutdown() {
  if (!impl_) return;
  stopMic();
  stopPassthrough();
  if (impl_->ctxOk) ma_context_uninit(&impl_->ctx);
  impl_->ctxOk = false;
}

std::string AudioEngine::backendName() const {
  if (!impl_->ctxOk) return "none";
  return ma_get_backend_name(impl_->ctx.backend);
}

void AudioEngine::refreshDevices() {
  if (!impl_->ctxOk) return;
  ma_device_info* play = nullptr;
  ma_device_info* cap = nullptr;
  ma_uint32 nPlay = 0, nCap = 0;
  if (ma_context_get_devices(&impl_->ctx, &play, &nPlay, &cap, &nCap) != MA_SUCCESS) return;
  std::vector<AudioDeviceInfo> ins, outs;
  std::map<std::string, ma_device_id> inIds, outIds;
  auto build = [](ma_device_info* list, ma_uint32 n, bool capture, std::vector<AudioDeviceInfo>& v,
                  std::map<std::string, ma_device_id>& ids) {
    DuplicateNamer namer;
    std::vector<int> keep;
    for (ma_uint32 i = 0; i < n; ++i) {
      std::string nm = list[i].name;
      // PulseAudio/PipeWire expose "Monitor of <sink>" loopback sources; they
      // are never a game-audio or microphone input, so they are not listed.
      if (capture && nm.rfind("Monitor of ", 0) == 0) continue;
      keep.push_back((int)i);
    }
    for (int i : keep) {
      AudioDeviceInfo d;
      d.name = list[i].name;
      d.id = namer(d.name);
      d.isDefault = list[i].isDefault != 0;
      ids[d.id] = list[i].id;
      v.push_back(d);
    }
  };
  build(cap, nCap, true, ins, inIds);
  build(play, nPlay, false, outs, outIds);
  std::lock_guard<std::mutex> lk(mu_);
  inputs_ = std::move(ins);
  outputs_ = std::move(outs);
  impl_->inIds = std::move(inIds);
  impl_->outIds = std::move(outIds);
}

std::vector<AudioDeviceInfo> AudioEngine::inputs() const {
  std::lock_guard<std::mutex> lk(mu_);
  return inputs_;
}
std::vector<AudioDeviceInfo> AudioEngine::outputs() const {
  std::lock_guard<std::mutex> lk(mu_);
  return outputs_;
}

bool AudioEngine::startPassthrough(const std::string& inId, const std::string& outId, std::string& err) {
  stopPassthrough();
  if (!impl_->ctxOk) {
    err = "audio backend unavailable";
    return false;
  }
  ma_device_id inDev{}, outDev{};
  bool haveIn = false, haveOut = false;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto a = impl_->inIds.find(inId);
    if (!inId.empty() && a != impl_->inIds.end()) inDev = a->second, haveIn = true;
    auto b = impl_->outIds.find(outId);
    if (!outId.empty() && outId != "default" && b != impl_->outIds.end()) outDev = b->second, haveOut = true;
  }
  ma_device_config c = ma_device_config_init(ma_device_type_duplex);
  c.capture.pDeviceID = haveIn ? &inDev : nullptr;
  c.capture.format = ma_format_f32;
  c.capture.channels = kChannels;
  c.capture.shareMode = ma_share_mode_shared;
  c.playback.pDeviceID = haveOut ? &outDev : nullptr;
  c.playback.format = ma_format_f32;
  c.playback.channels = kChannels;
  c.sampleRate = kRate;
  c.performanceProfile = ma_performance_profile_low_latency;
  c.periodSizeInMilliseconds = 10;
  c.periods = 3;
  // Android: raw capture (no AGC / noise suppression) and game-audio usage.
  c.aaudio.inputPreset = ma_aaudio_input_preset_unprocessed;
  c.aaudio.usage = ma_aaudio_usage_game;
  c.noPreSilencedOutputBuffer = MA_TRUE;
  c.noClip = MA_FALSE;
  c.dataCallback = duplexCb;
  c.pUserData = this;
  if (ma_device_init(&impl_->ctx, &c, &impl_->duplex) != MA_SUCCESS) {
    err = "could not open audio input/output";
    return false;
  }
  if (ma_device_start(&impl_->duplex) != MA_SUCCESS) {
    ma_device_uninit(&impl_->duplex);
    err = "could not start audio";
    return false;
  }
  impl_->duplexOk = true;
  return true;
}

bool AudioEngine::startPassthroughFrom(std::shared_ptr<SampleRing> src, int channels, const std::string& outId,
                                       std::string& err) {
  stopPassthrough();
  if (!impl_->ctxOk || !src) {
    err = "audio backend unavailable";
    return false;
  }
  ma_device_id outDev{};
  bool haveOut = false;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto b = impl_->outIds.find(outId);
    if (!outId.empty() && outId != "default" && b != impl_->outIds.end()) outDev = b->second, haveOut = true;
  }
  extCh_ = channels == 6 ? 6 : 2;
  tapMix_.assign(4096 * 2, 0.f);
  ext_ = std::move(src);
  ext_->clear();
  extPrimed_ = false;
  extLost_ = false;
  ma_device_config c = ma_device_config_init(ma_device_type_playback);
  c.playback.pDeviceID = haveOut ? &outDev : nullptr;
  c.playback.format = ma_format_f32;
  c.playback.channels = (ma_uint32)extCh_;
  c.sampleRate = kRate;
  c.performanceProfile = ma_performance_profile_low_latency;
  c.periodSizeInMilliseconds = 10;
  c.periods = 3;
  c.noPreSilencedOutputBuffer = MA_TRUE;
  c.dataCallback = playCb;
  c.pUserData = this;
  if (ma_device_init(&impl_->ctx, &c, &impl_->play) != MA_SUCCESS) {
    ext_.reset();
    err = "could not open audio output";
    return false;
  }
  if (ma_device_start(&impl_->play) != MA_SUCCESS) {
    ma_device_uninit(&impl_->play);
    ext_.reset();
    err = "could not start audio";
    return false;
  }
  impl_->playOk = true;
  return true;
}

void AudioEngine::stopPassthrough() {
  if (impl_ && impl_->duplexOk) {
    ma_device_uninit(&impl_->duplex);
    impl_->duplexOk = false;
  }
  if (impl_ && impl_->playOk) {
    ma_device_uninit(&impl_->play);
    impl_->playOk = false;
    ext_.reset();
  }
  extLost_ = false;
}

bool AudioEngine::passthroughRunning() const {
  return impl_->duplexOk || impl_->playOk;
}

bool AudioEngine::startMic(const std::string& id, std::string& err) {
  stopMic();
  if (!impl_->ctxOk) {
    err = "audio backend unavailable";
    return false;
  }
  ma_device_id dev{};
  bool have = false;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto a = impl_->inIds.find(id);
    if (!id.empty() && a != impl_->inIds.end()) dev = a->second, have = true;
  }
  ma_device_config c = ma_device_config_init(ma_device_type_capture);
  c.capture.pDeviceID = have ? &dev : nullptr;
  c.capture.format = ma_format_f32;
  c.capture.channels = kChannels;
  c.sampleRate = kRate;
  c.performanceProfile = ma_performance_profile_low_latency;
  c.periodSizeInMilliseconds = 10;
  c.dataCallback = micCb;
  c.pUserData = this;
  if (ma_device_init(&impl_->ctx, &c, &impl_->mic) != MA_SUCCESS) {
    err = "could not open microphone";
    return false;
  }
  if (ma_device_start(&impl_->mic) != MA_SUCCESS) {
    ma_device_uninit(&impl_->mic);
    err = "could not start microphone";
    return false;
  }
  impl_->micOk = true;
  return true;
}

void AudioEngine::stopMic() {
  if (impl_ && impl_->micOk) {
    ma_device_uninit(&impl_->mic);
    impl_->micOk = false;
  }
}

bool AudioEngine::micRunning() const { return impl_->micOk; }

void AudioEngine::setTap(bool on) {
  if (on) {
    game_.clear();
    mic_.clear();
  }
  tap_ = on;
}

void AudioEngine::onDuplex(float* out, const float* in, uint32_t frames) {
  size_t n = (size_t)frames * kChannels;
  float v = volume_.load(std::memory_order_relaxed);
  if (in) {
    for (size_t i = 0; i < n; ++i) out[i] = in[i] * v;
    if (tap_.load(std::memory_order_relaxed)) game_.write(in, n);
  } else {
    std::memset(out, 0, n * sizeof(float));
  }
}

// Plays from the external ring with a small, bounded latency: waits for
// ~30 ms before starting, drops the excess above ~80 ms (clock drift between
// the card and the headset), fills gaps with silence.
void AudioEngine::onPlay(float* out, uint32_t frames) {
  const size_t ch = (size_t)extCh_;
  size_t n = (size_t)frames * ch;
  SampleRing* r = ext_.get();
  if (!r) {
    std::memset(out, 0, n * sizeof(float));
    return;
  }
  const size_t start = kRate * ch * 30 / 1000, high = kRate * ch * 80 / 1000;
  size_t avail = r->available();
  if (!extPrimed_) {
    if (avail < start) {
      std::memset(out, 0, n * sizeof(float));
      return;
    }
    extPrimed_ = true;
  }
  if (avail > high + n) r->discard(avail - (start + n));
  size_t got = r->read(out, n);
  if (got < n) {
    std::memset(out + got, 0, (n - got) * sizeof(float));
    extPrimed_ = false;  // underrun: build up the cushion again
  }
  if (tap_.load(std::memory_order_relaxed)) {
    if (ch == 2) {
      game_.write(out, n);
    } else {
      // 5.1 → stereo for the recording (LFE left out).
      const float k = 0.7071f;
      for (size_t f0 = 0; f0 < frames;) {
        size_t m = std::min<size_t>(frames - f0, tapMix_.size() / 2);
        for (size_t i = 0; i < m; ++i) {
          const float* s = out + (f0 + i) * 6;
          tapMix_[2 * i] = std::clamp(s[0] + k * s[2] + k * s[4], -1.f, 1.f);
          tapMix_[2 * i + 1] = std::clamp(s[1] + k * s[2] + k * s[5], -1.f, 1.f);
        }
        game_.write(tapMix_.data(), m * 2);
        f0 += m;
      }
    }
  }
  float v = volume_.load(std::memory_order_relaxed);
  if (v != 1.f)
    for (size_t i = 0; i < n; ++i) out[i] *= v;
}

void AudioEngine::onMic(const float* in, uint32_t frames) {
  if (in && tap_.load(std::memory_order_relaxed)) mic_.write(in, (size_t)frames * kChannels);
}
