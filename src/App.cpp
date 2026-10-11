// UVC Studio behaviour: session, devices, video mode, audio, webcam overlay,
// picture controls and the GPU work done before each interface frame.
// Drawing lives in AppUi.cpp; snapshots, recordings and keys in
// AppActions.cpp.
#include "App.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>

#include "I18n.h"
#include "Icons.h"
#include "Ui.h"
#include "gl.h"

namespace {

std::string toLower(std::string s) {
  for (char& c : s) c = (char)std::tolower((unsigned char)c);
  return s;
}

bool hasAny(const std::string& lowerName, std::initializer_list<const char*> words) {
  for (const char* w : words)
    if (lowerName.find(w) != std::string::npos) return true;
  return false;
}

// Words that mark a capture card (§3.1).
const std::initializer_list<const char*> kCaptureWords = {"capture",       "hdmi",      "cam link", "game",
                                                          "video grabber", "usb video", "uvc",      "4k"};

// Words of an audio device name that say something about which hardware it
// is, for pairing the capture card's audio input with its video device.
std::set<std::string> nameTokens(const std::string& name) {
  static const std::set<std::string> generic = {
      "analog", "stereo", "digital", "input", "audio",  "mono",   "usb",   "multichannel", "surround", "iec958",
      "microphone", "mic", "the", "device", "source", "video", "camera", "front", "rear", "line"};
  std::set<std::string> out;
  std::string word;
  for (char c : toLower(name) + " ") {
    if (std::isalnum((unsigned char)c)) {
      word += c;
      continue;
    }
    if (!word.empty() && !generic.count(word)) out.insert(word);
    word.clear();
  }
  return out;
}

std::string modeString(int w, int h, int fps) {
  return std::to_string(w) + "x" + std::to_string(h) + "@" + std::to_string(fps);
}

bool parseMode(const std::string& s, int& w, int& h, int& fps) {
  return std::sscanf(s.c_str(), "%dx%d@%d", &w, &h, &fps) == 3 && w > 0 && h > 0 && fps > 0;
}

// Overlay camera qualities (§5).
struct OverlayQuality {
  int w, h, fps;
  const char* label;
};
const OverlayQuality kOverlayQuality[App::kOverlayQualities] = {
    {1280, 720, 30, "720p \xC2\xB7 30 fps"},
    {1920, 1080, 30, "1080p \xC2\xB7 30 fps"},
    {1920, 1080, 60, "1080p \xC2\xB7 60 fps"},
};

const App::PicturePreset kPresets[App::kPicturePresets] = {
    // brightness, contrast, saturation (−40 … +40), gamma
    {"preset.natural", 0, 0, 0, 1.00f},
    // Realism: a gentle S-curve feel (a little more contrast, mid-tones
    // lifted a touch so shadows keep their detail) with restrained colour.
    {"preset.realism", 0, 6, -8, 1.04f},
    {"preset.punchy", 2, 12, 25, 1.00f},
    {"preset.film", -3, 8, -20, 1.00f},
    // Bright room: the mid-tones lifted as well, so dark scenes stay legible.
    {"preset.brightRoom", 8, 5, 5, 1.10f},
};

constexpr int kPictureRange = 40;  // shown as −40 … +40

}  // namespace

// ===========================================================================
// Static helpers
// ===========================================================================
std::string App::resolutionText(int w, int h) { return std::to_string(w) + " \xC3\x97 " + std::to_string(h); }

std::string App::resolutionTag(int w, int h) {
  if (w >= 3840) return "4K";
  if (w == 2560 && h == 1440) return "QHD";
  if (w == 1920 && h == 1080) return "Full HD";
  if (w == 1280 && h == 720) return "HD";
  if (w == 720 && (h == 480 || h == 576)) return "SD";
  return "";
}

std::string App::resolutionLabel(int w, int h) {
  std::string tag = resolutionTag(w, h);
  return tag.empty() ? resolutionText(w, h) : resolutionText(w, h) + " \xC2\xB7 " + tag;
}

std::string App::fpsLabel(int fps) { return std::to_string(fps) + " fps"; }

const char* App::overlayQualityLabel(int q) { return kOverlayQuality[std::clamp(q, 0, kOverlayQualities - 1)].label; }

const App::PicturePreset& App::picturePreset(int i) { return kPresets[std::clamp(i, 0, kPicturePresets - 1)]; }

int64_t App::recordingBitrate(int w, int h, int fps) {
  double bits = (double)w * h * fps * 0.12;
  return (int64_t)std::clamp(bits, 8e6, 50e6);
}

void App::overlayRectFor(int corner, int size, float videoAspect, float r[4]) {
  static const float kWidth[3] = {0.18f, 0.26f, 0.36f};
  const float margin = 0.03f;
  float aspect = videoAspect > 0 ? videoAspect : 16.f / 9.f;
  float w = kWidth[std::clamp(size, 0, 2)];
  // Normalised height of a 16:9 box that is w of the video width wide.
  float h = w * 9.f / 16.f * aspect;
  float my = margin * aspect;
  h = std::min(h, 1.f - 2 * my);
  bool left = corner == CornerTopLeft || corner == CornerBottomLeft;
  bool top = corner == CornerTopLeft || corner == CornerTopRight;
  r[0] = left ? margin : 1.f - margin - w;
  r[1] = top ? my : 1.f - my - h;
  r[2] = r[0] + w;
  r[3] = r[1] + h;
}

int App::nearestCorner(float nx, float ny) {
  bool left = nx < 0.5f, top = ny < 0.5f;
  if (top) return left ? CornerTopLeft : CornerTopRight;
  return left ? CornerBottomLeft : CornerBottomRight;
}

int App::sourceScore(const VideoDeviceInfo& d) {
  std::string n = toLower(d.name);
  int s = 0;
  if (hasAny(n, kCaptureWords)) s += 4;
  if (hasAny(n, {"virtual", "loopback"}) || toLower(d.driver) == "v4l2 loopback") s -= 6;
  if (hasAny(n, {"integrated", "built-in", "facetime", "webcam", "face", "front", "rear", "ir camera"})) s -= 3;
  return s;
}

int App::overlayScore(const VideoDeviceInfo& d) {
  std::string n = toLower(d.name);
  int s = 0;
  if (hasAny(n, {"webcam", "camera", "cam", "facetime", "integrated", "built-in"})) s += 4;
  if (hasAny(n, {"virtual", "loopback"}) || toLower(d.driver) == "v4l2 loopback") s -= 6;
  if (hasAny(n, kCaptureWords)) s -= 4;
  return s;
}

// ===========================================================================
// Start-up and shut-down
// ===========================================================================
bool App::init(SDL_Window* win) {
  win_ = win;
  settings_.load();
  lang_ = i18n::pickLanguage(settings_.get("ui.lang"), platform::preferredLanguage());

  themePref_ = settings_.get("ui.theme", "auto");
  if (themePref_ != "dark" && themePref_ != "light") themePref_ = "auto";
  uiScalePct_ = (int)settings_.getNum("ui.scale", 0);
  uiZoom_ = (float)std::clamp(settings_.getNum("ui.zoom", 1), 0.6, 2.5);
  loadCardLayout();

  savedVideo_ = settings_.get("source.device");
  if (!parseMode(settings_.get("source.mode"), savedW_, savedH_, savedFps_)) savedW_ = savedH_ = savedFps_ = 0;
  formatPref_ = (uint32_t)std::clamp((int)settings_.getNum("source.format", PREFER_AUTO), 0, (int)PREFER_COMPRESSED);

  savedAudioIn_ = settings_.get("sound.input");
  selAudioOut_ = settings_.get("sound.output", "default");
  listen_ = settings_.getNum("sound.listen", 1) != 0;
  volume_ = (float)std::clamp(settings_.getNum("sound.volume", 1), 0.0, 1.0);
  micSource_ = settings_.get("sound.micSource");
  micRec_ = settings_.getNum("sound.mic", 0) != 0;
  gameChannels_ = settings_.getNum("sound.channels", 2) >= 6 ? 6 : 2;

  auto gain = [&](const char* key) {
    int v = std::clamp((int)std::lround(settings_.getNum(key, 0)), -kPictureRange, kPictureRange);
    return 1.f + v / 100.f;
  };
  color_.brightness = gain("picture.brightness");
  color_.contrast = gain("picture.contrast");
  color_.saturation = gain("picture.saturation");
  {
    float g = (float)settings_.getNum("picture.gamma", 1.0);
    color_.gamma = std::round(std::clamp(g, kGammaMin, kGammaMax) * 100.f) / 100.f;
  }
  upscaleMode_ = std::clamp((int)settings_.getNum("picture.upscaler", 0), 0, kUpscaleModes - 1);
  retroStyle_ = std::clamp((int)settings_.getNum("picture.retro", RetroOff), (int)RetroOff, (int)RetroArcade);
  if (retroStyle_ != RetroOff) retroLast_ = retroStyle_;

  overlayOn_ = settings_.getNum("overlay.on", 0) != 0;
  savedOverlayCam_ = settings_.get("overlay.camera");
  overlayQuality_ = std::clamp((int)settings_.getNum("overlay.quality", 0), 0, kOverlayQualities - 1);
  overlaySize_ = std::clamp((int)settings_.getNum("overlay.size", OverlayMedium), 0, 2);
  overlayCorner_ = std::clamp((int)settings_.getNum("overlay.corner", CornerBottomLeft), 0, 3);
  overlayMirror_ = settings_.getNum("overlay.mirror", 0) != 0;

  autoConnect_ = settings_.getNum("general.autoConnect", 1) != 0;

  vrPassthrough_ = settings_.getNum("vr.passthrough", 1) != 0;
  vrScreenWidth_ = (float)settings_.getNum("vr.width", 2.6);
  vrScreenDist_ = (float)settings_.getNum("vr.distance", 2.2);
  for (int i = 0; i < 2; ++i) {
    std::string k = i ? "vr.panel" : "vr.panelIdle";
    VrPlace d = defaultVrPanel(i == 1), p;
    p.yaw = (float)settings_.getNum(k + "Yaw", d.yaw);
    p.height = (float)settings_.getNum(k + "Height", d.height);
    p.dist = (float)settings_.getNum(k + "Distance", d.dist);
    setVrPanelPlace(i == 1, p, false);
  }
  vrRefresh_ = (int)settings_.getNum("vr.refresh", 90);
  vrCurved_ = settings_.getNum("vr.curved", 0) != 0;
  depthStrength_ = (float)std::clamp(settings_.getNum("depth.strength", 0.6), 0.1, 1.0);
  {
    const int maxFormat = DepthAnaglyph;
    int f = std::clamp((int)settings_.getNum("depth.format", DepthNone), (int)DepthNone, maxFormat);
    depthLast_ = std::clamp((int)settings_.getNum("depth.lastFormat", DepthEffect), (int)DepthEffect, maxFormat);
    if (f != DepthNone) depthLast_ = f;
    depth3d_ = f != DepthNone;
    stereoOut_ = depth3d_ ? f - DepthEffect : StereoOff;
  }

  std::string err;
  if (!audio_.init(err)) setError("err.gameAudio", err);
  applyVolume();
  if (!GpuPrograms::get().init()) std::fprintf(stderr, "[uvc-studio] video shaders failed to build\n");
  if (!DepthPrograms::get().init()) std::fprintf(stderr, "[uvc-studio] 3D depth shaders unavailable\n");

  // First device list synchronously (this also auto-connects at launch),
  // then a background watcher for devices coming and going.
  {
    std::lock_guard<std::mutex> lk(devMu_);
    pendingVideo_ = enumerateVideoDevices();
    pendingIns_ = audio_.inputs();
    pendingOuts_ = audio_.outputs();
  }
  devVersion_ = 1;
  applyDeviceList();
  startWatcher();
  return true;
}

void App::startWatcher() {
  watcher_ = std::thread([this] {
    std::vector<VideoDeviceInfo> video;
    {
      std::lock_guard<std::mutex> lk(devMu_);
      video = pendingVideo_;
    }
    std::string videoSig = videoDeviceSignature();
    std::string listSig;
    double audioScanAt = nowSeconds();
    while (!watcherStop_) {
      // Look again every 2 s, or at once when asked to.
      for (int i = 0; i < 20 && !watcherStop_ && !refreshNow_; ++i) SDL_Delay(100);
      if (watcherStop_) break;
      bool forced = refreshNow_.exchange(false);
      std::string sig = videoDeviceSignature();
      if (forced || sig != videoSig) {
        video = enumerateVideoDevices();
        videoSig = sig;
      }
      double t = nowSeconds();
      if (forced || t - audioScanAt > 4.0) {
        audio_.refreshDevices();
        audioScanAt = t;
      }
      std::vector<AudioDeviceInfo> ins = audio_.inputs(), outs = audio_.outputs();
      // Quest lists a card before USB access is granted, without modes.
      std::string s;
      for (auto& v : video) s += v.id + "#" + std::to_string(v.modes.size()) + "|";
      for (auto& a : ins) s += "i:" + a.id + "|";
      for (auto& a : outs) s += "o:" + a.id + "|";
      if (!forced && s == listSig) continue;
      listSig = s;
      std::lock_guard<std::mutex> lk(devMu_);
      pendingVideo_ = video;
      pendingIns_ = std::move(ins);
      pendingOuts_ = std::move(outs);
      ++devVersion_;
    }
  });
}

void App::shutdown() {
  watcherStop_ = true;
  if (watcher_.joinable()) watcher_.join();
  if (recording_) stopRecording();
  // Let the recorder finish the file and publish it (Quest: into the
  // gallery) — the app may be closing or switching between 2D and VR.
  for (int i = 0; i < 300 && recorder_.busy(); ++i) {
    std::string path, err;
    bool ok = false;
    if (recorder_.pollFinished(path, ok, err)) {
      if (ok) platform::publishOutput(path, "video/mp4");
      break;
    }
    SDL_Delay(20);
  }
  // Background snapshot / gallery copies refer to this App.
  for (int i = 0; i < 1500 && publishing_.load() > 0; ++i) SDL_Delay(20);
  closeAll();
  connected_ = false;
  readback_.reset();
  compose_.release();
  xrScreen_.release();
  xrDepth_.release();
  stereoDepth_.release();
  for (GpuImage* g : {&stereoSrc_, &stereoImg_, &stereoL_, &stereoR_}) g->release();
  videoTimer_.release();
  stereoTimer_.release();
  backdrop_.release();
  icons::clear();
  DepthPrograms::get().shutdown();
  GpuPrograms::get().shutdown();
  audio_.shutdown();
}

// ===========================================================================
// Errors
// ===========================================================================
void App::setError(const char* key, const std::string& detail) {
  errKey_ = key;
  errDetail_ = detail;
  errAt_ = nowSeconds();
  std::fprintf(stderr, "[uvc-studio] %s %s\n", key, detail.c_str());
}

void App::clearError() {
  errKey_.clear();
  errDetail_.clear();
}

std::string App::errorText() const {
  if (errKey_.empty() || nowSeconds() - errAt_ > kErrorSeconds) return "";
  std::string text = i18n::tr(lang_, errKey_.c_str());
  size_t p = text.find("{detail}");
  if (p != std::string::npos) text.replace(p, 8, errDetail_);
  return text;
}

// ===========================================================================
// Devices
// ===========================================================================
const VideoDeviceInfo* App::findVideo(const std::string& id) const {
  if (id.empty()) return nullptr;
  for (auto& v : videoDevs_)
    if (v.id == id) return &v;
  return nullptr;
}

std::string App::currentVideoLabel() const {
  const VideoDeviceInfo* d = currentVideoDev();
  return d ? d->name : std::string();
}

std::string App::currentAudioLabel() const {
  for (auto& a : audioIns_)
    if (a.id == selAudioIn_) return a.id;
  return "";
}

void App::pickSource() {
  if (findVideo(savedVideo_)) {
    selVideo_ = savedVideo_;
    return;
  }
  selVideo_.clear();
  int best = 0;
  for (auto& v : videoDevs_) {
    int s = sourceScore(v);
    if (selVideo_.empty() || s > best) {  // ties keep list order
      best = s;
      selVideo_ = v.id;
    }
  }
}

void App::pickGameAudio() {
  for (auto& a : audioIns_)
    if (!savedAudioIn_.empty() && a.id == savedAudioIn_) {
      selAudioIn_ = savedAudioIn_;
      return;
    }
  selAudioIn_.clear();
  // The input whose name shares the most words with the source's name.
  std::set<std::string> videoWords = nameTokens(currentVideoLabel());
  int best = 0;
  for (auto& a : audioIns_) {
    int score = 0;
    for (auto& w : nameTokens(a.name))
      if (videoWords.count(w)) score += 2;
    if (score > best) {
      best = score;
      selAudioIn_ = a.id;
    }
  }
}

void App::pickOverlayCamera() {
  const VideoDeviceInfo* saved = findVideo(savedOverlayCam_);
  if (saved && saved->id != selVideo_) {
    overlayCam_ = savedOverlayCam_;
    return;
  }
  overlayCam_.clear();
  int best = 0;
  for (auto& v : videoDevs_) {
    if (v.id == selVideo_) continue;
    int s = overlayScore(v);
    if (overlayCam_.empty() || s > best) {
      best = s;
      overlayCam_ = v.id;
    }
  }
}

void App::applyDeviceList() {
  {
    std::lock_guard<std::mutex> lk(devMu_);
    videoDevs_ = pendingVideo_;
    audioIns_ = pendingIns_;
    audioOuts_ = pendingOuts_;
  }
  appliedVersion_ = devVersion_;

  // Devices that became openable since the previous list.
  std::set<std::string> openable;
  std::vector<std::string> appeared;
  for (auto& v : videoDevs_) {
    if (v.modes.empty()) continue;
    openable.insert(v.id);
    if (!firstDeviceList_ && !openableVideo_.count(v.id)) appeared.push_back(v.id);
  }
  openableVideo_ = std::move(openable);

  // While connected the devices in use stay as they are.
  if (!connected_) {
    pickSource();
    resolveMode();
    pickGameAudio();
  }
  if (!(connected_ && overlayCap_.isOpen())) pickOverlayCamera();

  bool first = firstDeviceList_;
  firstDeviceList_ = false;
  if (connected_ || !autoConnect_) return;
  auto likely = [&](const VideoDeviceInfo& d) { return d.id == savedVideo_ || sourceScore(d) > 0; };
  bool go = false;
  if (first) {
    const VideoDeviceInfo* cur = currentVideoDev();
    go = cur && likely(*cur);
    go = go && openableVideo_.count(cur->id);
  } else {
    for (const std::string& id : appeared) {
      const VideoDeviceInfo* d = findVideo(id);
      if (!d || !likely(*d)) continue;
      const VideoDeviceInfo* cur = currentVideoDev();
      if (!cur || !likely(*cur) || !openableVideo_.count(cur->id)) {
        selVideo_ = id;
        resolveMode();
        pickGameAudio();
        pickOverlayCamera();
      }
      go = true;
    }
  }
  if (go) connect();
}

void App::setSourceDevice(const std::string& id) {
  savedVideo_ = id;
  settings_.set("source.device", id);
  pickSource();
  resolveMode();
  if (savedAudioIn_.empty()) pickGameAudio();
  if (!(connected_ && overlayCap_.isOpen()) || overlayCam_ == selVideo_) pickOverlayCamera();
  if (!connected_) return;
  reopenVideo();
  restartAudio();
  if (overlayOn_ && !overlayCap_.isOpen()) openOverlay();
}

void App::setGameAudioInput(const std::string& id) {
  savedAudioIn_ = id;
  settings_.set("sound.input", id);
  pickGameAudio();
  restartAudio();
}

void App::setAudioOutput(const std::string& id) {
  selAudioOut_ = id.empty() ? "default" : id;
  settings_.set("sound.output", selAudioOut_);
  restartAudio();
}

void App::setMicSource(const std::string& id) {
  micSource_ = id;
  settings_.set("sound.micSource", id);
  if (connected_ && micRec_) startMic();
}

// ===========================================================================
// Video mode
// ===========================================================================
std::vector<std::pair<int, int>> App::sourceResolutions() const {
  const VideoDeviceInfo* d = currentVideoDev();
  return d ? d->resolutions() : std::vector<std::pair<int, int>>{};
}

std::vector<int> App::sourceFrameRates(int w, int h) const {
  const VideoDeviceInfo* d = currentVideoDev();
  return d ? d->frameRates(w, h) : std::vector<int>{};
}

// The frame rate to use at a resolution when the wanted one is not offered:
// 60 when available, otherwise the highest.
static int preferredRate(const std::vector<int>& rates, int wanted) {
  if (rates.empty()) return wanted > 0 ? wanted : 60;
  if (std::find(rates.begin(), rates.end(), wanted) != rates.end()) return wanted;
  if (std::find(rates.begin(), rates.end(), 60) != rates.end()) return 60;
  return rates.front();
}

void App::resolveMode() {
  auto resolutions = sourceResolutions();
  if (resolutions.empty()) {
    // Nothing reported (yet): the saved mode, or Full HD 60.
    if (savedW_ > 0) modeW_ = savedW_, modeH_ = savedH_, modeFps_ = savedFps_;
    return;
  }
  if (savedW_ > 0) {
    auto rates = sourceFrameRates(savedW_, savedH_);
    if (std::find(rates.begin(), rates.end(), savedFps_) != rates.end()) {
      modeW_ = savedW_, modeH_ = savedH_, modeFps_ = savedFps_;
      return;
    }
  }
  // Default: the largest resolution offering ≥ 59 fps, else the largest
  // resolution at its highest rate.
  for (auto& r : resolutions) {
    auto rates = sourceFrameRates(r.first, r.second);
    if (!rates.empty() && rates.front() >= 59) {
      modeW_ = r.first, modeH_ = r.second;
      modeFps_ = preferredRate(rates, 0);
      return;
    }
  }
  modeW_ = resolutions.front().first, modeH_ = resolutions.front().second;
  auto rates = sourceFrameRates(modeW_, modeH_);
  modeFps_ = rates.empty() ? 30 : rates.front();
}

void App::setSourceResolution(int w, int h) {
  modeW_ = w, modeH_ = h;
  modeFps_ = preferredRate(sourceFrameRates(w, h), modeFps_);
  savedW_ = modeW_, savedH_ = modeH_, savedFps_ = modeFps_;
  settings_.set("source.mode", modeString(modeW_, modeH_, modeFps_));
  reopenVideo();
}

void App::setSourceFrameRate(int fps) {
  modeFps_ = fps;
  savedW_ = modeW_, savedH_ = modeH_, savedFps_ = modeFps_;
  settings_.set("source.mode", modeString(modeW_, modeH_, modeFps_));
  reopenVideo();
}

void App::setSourceFormat(uint32_t pref) {
  formatPref_ = pref;
  settings_.setNum("source.format", pref);
  reopenVideo();
}

// ===========================================================================
// Session
// ===========================================================================
void App::setAutoConnect(bool on) {
  autoConnect_ = on;
  settings_.setNum("general.autoConnect", on ? 1 : 0);
}

void App::openSource() {
  const VideoDeviceInfo* dev = currentVideoDev();
  if (!dev) return;
  StreamSettings want = chooseMode(*dev, modeW_, modeH_, modeFps_, formatPref_);
  std::string err;
  lastSeq_ = 0;
  if (!cap_.open(*dev, want, err)) {
    setError("err.sourceOpen", err);
    return;
  }
  streamActual_ = cap_.actual();
  streamStartT_ = nowSeconds();
  std::printf("[uvc-studio] source %s %dx%d@%.2f %s\n", dev->name.c_str(), streamActual_.width, streamActual_.height,
              streamActual_.fps, fourccName(streamActual_.fourcc).c_str());
}

void App::connect() {
  if (connected_) return;
  if (!currentVideoDev()) pickSource();
  const VideoDeviceInfo* dev = currentVideoDev();
  if (!dev) {
    setError("err.noSource");
    return;
  }
  clearError();
  resolveMode();
  openSource();
  if (!cap_.isOpen()) return;
  connected_ = true;
  stallRetries_ = 0;
  stallReported_ = false;
  startGameAudio();
  if (micRec_) startMic();
  if (overlayOn_) openOverlay();
}

void App::closeAll() {
  closeOverlay();
  cap_.close();
  audio_.stopPassthrough();
  audio_.stopMic();
  pipe_.release();
  stereoReady_ = false;
  streamActual_ = StreamSettings{};
}

void App::disconnect() {
  if (!connected_) return;
  if (recording_) stopRecording();
  closeAll();
  connected_ = false;
  if (fullscreen_) toggleFullscreen();
}

void App::reopenVideo() {
  if (!connected_) return;
  if (recording_) stopRecording();  // the picture size may change
  cap_.close();
  openSource();
  if (!cap_.isOpen()) {
    closeAll();
    connected_ = false;
    return;
  }
}

// ===========================================================================
// Audio
// ===========================================================================
void App::applyVolume() {
  bool audible = listen_ && !(xrMode_ && !xrFocused_);
  audio_.setVolume(audible ? volume_ : 0.f);
}

void App::startGameAudio() {
  audio_.stopPassthrough();
  if (!connected_ || selAudioIn_.empty()) return;  // video only
  std::string err;
  bool ok;
  ok = audio_.startPassthrough(selAudioIn_, selAudioOut_, err);
  if (!ok) setError("err.gameAudio", err);
  applyVolume();
}

void App::restartAudio() {
  if (!connected_) return;
  startGameAudio();
}

void App::startMic() {
  audio_.stopMic();
  if (!connected_ || !micRec_) return;
  std::string err;
  if (!audio_.startMic(micSource_, err)) setError("err.mic", err);
}

void App::setListen(bool on) {
  listen_ = on;
  settings_.setNum("sound.listen", on ? 1 : 0);
  applyVolume();
}


void App::setGameChannels(int n) {
  n = n >= 6 ? 6 : 2;
  if (n == gameChannels_) return;
  gameChannels_ = n;
  settings_.setNum("sound.channels", n);
}

void App::setListenVolume(float v) {
  volume_ = std::clamp(v, 0.f, 1.f);
  settings_.setNum("sound.volume", volume_);
  applyVolume();
}

void App::setMicRecording(bool on) {
  micRec_ = on;
  settings_.setNum("sound.mic", on ? 1 : 0);
  if (on)
    startMic();
  else
    audio_.stopMic();
  recorder_.setMic(on);
}

// ===========================================================================
// Webcam overlay
// ===========================================================================
bool App::overlayRectNorm(float r[4]) const {
  if (!overlayOn_ || !connected_ || !pipe_.hasImage() || !overlayPipe_.hasImage()) return false;
  if (overlayHeld_) {
    std::copy(overlayHeldRect_, overlayHeldRect_ + 4, r);
    return true;
  }
  overlayRectFor(overlayCorner_, overlaySize_, (float)pipe_.src().w / pipe_.src().h, r);
  return true;
}

void App::setOverlayDisplayRect(const float* r) {
  overlayHeld_ = r != nullptr;
  if (r) std::copy(r, r + 4, overlayHeldRect_);
}

void App::openOverlay() {
  closeOverlay();
  if (!connected_ || !overlayOn_) return;
  if (overlayCam_.empty() || overlayCam_ == selVideo_) pickOverlayCamera();
  const VideoDeviceInfo* dev = findVideo(overlayCam_);
  std::string err;
  if (!dev) {
    err = i18n::tr(lang_, "err.noWebcam");
  } else {
    const OverlayQuality& q = kOverlayQuality[overlayQuality_];
    StreamSettings want = chooseMode(*dev, q.w, q.h, q.fps, PREFER_AUTO);
    lastOverlaySeq_ = 0;
    if (overlayCap_.open(*dev, want, err)) return;
  }
  // Fails once: say so and switch the overlay off.
  setError("err.overlayCamera", err);
  overlayOn_ = false;
  settings_.setNum("overlay.on", 0);
}

void App::closeOverlay() {
  overlayCap_.close();
  overlayPipe_.release();
  overlayHeld_ = false;
}

void App::setOverlayOn(bool on) {
  overlayOn_ = on;
  settings_.setNum("overlay.on", on ? 1 : 0);
  if (on)
    openOverlay();
  else
    closeOverlay();
}

void App::setOverlayCorner(int corner) {
  overlayCorner_ = std::clamp(corner, 0, 3);
  overlayHeld_ = false;
  settings_.setNum("overlay.corner", overlayCorner_);
}

void App::setOverlaySize(int size) {
  overlaySize_ = std::clamp(size, 0, 2);
  settings_.setNum("overlay.size", overlaySize_);
}

void App::stepOverlaySize(int dir) { setOverlaySize(overlaySize_ + (dir > 0 ? 1 : dir < 0 ? -1 : 0)); }

void App::setOverlayMirror(bool on) {
  overlayMirror_ = on;
  settings_.setNum("overlay.mirror", on ? 1 : 0);
}

void App::setOverlayCamera(const std::string& id) {
  savedOverlayCam_ = id;
  settings_.set("overlay.camera", id);
  pickOverlayCamera();
  if (overlayOn_ && connected_) openOverlay();
}

void App::setOverlayQuality(int q) {
  overlayQuality_ = std::clamp(q, 0, kOverlayQualities - 1);
  settings_.setNum("overlay.quality", overlayQuality_);
  if (overlayOn_ && connected_) openOverlay();
}

bool App::canSwapPictures() const {
  return findVideo(selVideo_) && findVideo(overlayCam_) && overlayCam_ != selVideo_;
}

void App::swapPictures() {
  if (!canSwapPictures()) return;
  std::string newSource = overlayCam_, newCamera = selVideo_;
  savedVideo_ = selVideo_ = newSource;
  savedOverlayCam_ = overlayCam_ = newCamera;
  settings_.set("source.device", savedVideo_);
  settings_.set("overlay.camera", savedOverlayCam_);
  resolveMode();
  if (savedAudioIn_.empty()) pickGameAudio();
  if (!connected_) return;
  closeOverlay();
  reopenVideo();
  restartAudio();
  if (overlayOn_) openOverlay();
}

// ===========================================================================
// Picture
// ===========================================================================
int App::pictureValue(int which) const {
  float g = which == PicBrightness ? color_.brightness : which == PicContrast ? color_.contrast : color_.saturation;
  return (int)std::lround((g - 1.f) * 100.f);
}

void App::setPictureValue(int which, int v) {
  v = std::clamp(v, -kPictureRange, kPictureRange);
  which = std::clamp(which, (int)PicBrightness, (int)PicSaturation);
  static const char* kKeys[3] = {"picture.brightness", "picture.contrast", "picture.saturation"};
  float* target = which == PicBrightness ? &color_.brightness
                  : which == PicContrast ? &color_.contrast
                                         : &color_.saturation;
  *target = 1.f + v / 100.f;
  settings_.setNum(kKeys[which], v);
}

void App::setGamma(float g) {
  color_.gamma = std::round(std::clamp(g, kGammaMin, kGammaMax) * 100.f) / 100.f;
  settings_.setNum("picture.gamma", color_.gamma);
}

void App::applyPicturePreset(int i) {
  const PicturePreset& p = picturePreset(i);
  setPictureValue(PicBrightness, p.brightness);
  setPictureValue(PicContrast, p.contrast);
  setPictureValue(PicSaturation, p.saturation);
  setGamma(p.gamma);
}

int App::activePicturePreset() const {
  for (int i = 0; i < kPicturePresets; ++i) {
    const PicturePreset& p = kPresets[i];
    if (pictureValue(PicBrightness) == p.brightness && pictureValue(PicContrast) == p.contrast &&
        pictureValue(PicSaturation) == p.saturation && std::lround(color_.gamma * 100) == std::lround(p.gamma * 100))
      return i;
  }
  return -1;
}

void App::toggleMirror() { mirror_ = !mirror_; }

void App::toggleUpscale() { setUpscale(!upscale_); }

void App::setUpscale(bool on) {
  upscale_ = on;
}

void App::setUpscaleMode(int m) {
  upscaleMode_ = std::clamp(m, 0, kUpscaleModes - 1);
  settings_.setNum("picture.upscaler", upscaleMode_);
}

void App::setRetroStyle(int s) {
  retroStyle_ = s >= RetroCrt && s <= RetroArcade ? s : RetroOff;
  if (retroStyle_ != RetroOff) retroLast_ = retroStyle_;
  settings_.setNum("picture.retro", retroStyle_);
}

void App::toggleRetro() { setRetroStyle(retroStyle_ != RetroOff ? RetroOff : retroLast_); }

void App::cycleRetro() { setRetroStyle(retroStyle_ == RetroArcade ? RetroOff : retroStyle_ + 1); }

void App::setDepthFormat(int f) {
  const int maxFormat = DepthAnaglyph;
  f = std::clamp(f, (int)DepthNone, maxFormat);
  depth3d_ = f != DepthNone;
  if (depth3d_) {
    depthLast_ = f;
    settings_.setNum("depth.lastFormat", depthLast_);
    int out = f - DepthEffect;
    if (out != stereoOut_) stereoReady_ = false;
    stereoOut_ = out;
  }
  settings_.setNum("depth.format", f);
}

void App::toggleDepth() { setDepthFormat(depth3d_ ? (int)DepthNone : depthLast_); }

void App::setDepthStrength(float v) {
  depthStrength_ = std::clamp(v, 0.1f, 1.f);
  settings_.setNum("depth.strength", depthStrength_);
}

// ===========================================================================
// Quest
// ===========================================================================

int App::vrEffectiveWidth() const {
  float w = vrScreenWidth_, d = std::max(0.5f, vrScreenDist_);
  float rad = vrCurved_ ? w / d : 2.f * std::atan(w / (2.f * d));
  int px = (int)std::lround(rad * 57.2958f * 30.f);
  return std::clamp(px, 640, 2560);
}

void App::setVrScreen(float width, float dist, bool persist) {
  vrScreenDist_ = std::clamp(dist, 0.8f, 8.f);
  // A curved screen can wrap at most ~340° around the viewer.
  vrScreenWidth_ = std::clamp(width, 0.8f, vrCurved_ ? std::min(8.f, 5.9f * vrScreenDist_) : 8.f);
  if (persist) {
    settings_.setNum("vr.width", vrScreenWidth_);
    settings_.setNum("vr.distance", vrScreenDist_);
  }
}

void App::setVrPanelPlace(bool live, VrPlace p, bool persist) {
  p.yaw = std::clamp(p.yaw, -3.1f, 3.1f);
  p.height = std::clamp(p.height, -1.6f, 1.6f);
  p.dist = std::clamp(p.dist, 0.45f, 2.5f);
  vrPanel_[live ? 1 : 0] = p;
  if (persist) {
    std::string k = live ? "vr.panel" : "vr.panelIdle";
    settings_.setNum(k + "Yaw", p.yaw);
    settings_.setNum(k + "Height", p.height);
    settings_.setNum(k + "Distance", p.dist);
  }
}

bool App::vrPanelMoved() const {
  for (int i = 0; i < 2; ++i) {
    VrPlace a = vrPanel_[i], b = defaultVrPanel(i == 1);
    if (std::fabs(a.yaw - b.yaw) > 1e-3f || std::fabs(a.height - b.height) > 1e-3f || std::fabs(a.dist - b.dist) > 1e-3f)
      return true;
  }
  return false;
}

void App::resetVrPanel() {
  for (int i = 0; i < 2; ++i) setVrPanelPlace(i == 1, defaultVrPanel(i == 1), true);
}

// ===========================================================================
// Interface helpers
// ===========================================================================
void App::setLang(const std::string& l) {
  lang_ = l;
  settings_.set("ui.lang", l);
}

void App::zoom(float z) {
  uiZoom_ = std::clamp(z, 0.6f, 2.5f);
  settings_.setNum("ui.zoom", uiZoom_);
}

void App::toggleFullscreen() {
  if (!connected_ && !fullscreen_) return;
  fullscreen_ = !fullscreen_;
  if (!xrMode_ && win_) SDL_SetWindowFullscreen(win_, fullscreen_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void App::pushToast(const std::string& text, const std::string& path, bool error) {
  std::lock_guard<std::mutex> lk(toastMu_);
  toasts_.push_back({text, path, nowSeconds() + (error ? 8.0 : 5.0), error});
  if (toasts_.size() > 3) toasts_.erase(toasts_.begin());
}

void App::tick() {
  if (devVersion_ != appliedVersion_) applyDeviceList();
  applyVolume();
}

// ===========================================================================
// GPU work
// ===========================================================================
// The composed picture (picture chain output, mirror, overlay) at W × H —
// what snapshots, recordings, the VR screen and the stereo output use.
void App::composeFrame(GpuImage& target, int W, int H) {
  auto& P = GpuPrograms::get();
  target.ensure(W, H);
  GlStateGuard guard;
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
  glViewport(0, 0, W, H);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(P.blit);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(P.blit, "u_tex"), 0);
  glBindTexture(GL_TEXTURE_2D, pipe_.view().tex);
  float fw = (float)W, fh = (float)H;
  if (mirror_)
    P.quad(P.blit, W, H, 0, 0, fw, fh, 1, 0, 0, 1);
  else
    P.quad(P.blit, W, H, 0, 0, fw, fh, 0, 0, 1, 1);

  float r[4];
  if (!overlayRectNorm(r)) return;
  float x0 = r[0] * fw, y0 = r[1] * fh, x1 = r[2] * fw, y1 = r[3] * fh;
  const GpuImage& cam = overlayPipe_.view();
  // Centre-crop the camera to the box's shape.
  float boxAspect = (x1 - x0) / std::max(1.f, y1 - y0);
  float camAspect = (float)cam.w / std::max(1, cam.h);
  float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
  if (camAspect > boxAspect) {
    u0 = 0.5f * (1.f - boxAspect / camAspect);
    u1 = 1.f - u0;
  } else if (camAspect < boxAspect) {
    v0 = 0.5f * (1.f - camAspect / boxAspect);
    v1 = 1.f - v0;
  }
  if (overlayMirror_) std::swap(u0, u1);
  glBindTexture(GL_TEXTURE_2D, cam.tex);
  P.quad(P.blit, W, H, x0, y0, x1, y1, u0, v0, u1, v1);
}

void App::applyEffects() {
  static const Effect kUpscalers[kUpscaleModes] = {Effect::Upscale, Effect::Anime4K, Effect::LanczosCas, Effect::Nis};
  Effect upscaler = upscale_ ? kUpscalers[upscaleMode_] : Effect::None;
  // Size the picture is shown at, so the TV-style patterns match the display:
  // the VR screen spans about 1600 pixels, a flat screen the letterboxed stage.
  float aspect = (float)pipe_.src().w / pipe_.src().h;
  float sw = (stageMax_.x - stageMin_.x) * fbScale_, sh = (stageMax_.y - stageMin_.y) * fbScale_;
  if (xrMode_) sw = 1600.f, sh = 1600.f / aspect;
  float w = sw, h = sw / aspect;
  if (h > sh) h = sh, w = sh * aspect;
  pipe_.setDisplaySize((int)w, (int)h);
  // In VR the stereo pair already gives depth, so the flat depth enhancer
  // only runs on flat displays.
  float depth = depth3d_ && !xrMode_ ? depthStrength_ : 0.f;
  if (xrMode_) {
    int lw = vrEffectiveWidth();
    pipe_.setUpscaleLimit(lw, std::max(2, lw * 9 / 16) * 2);
  } else {
    pipe_.setUpscaleLimit(3840, 2160);
  }
  pipe_.setTime(nowSeconds());
  pipe_.process(upscaler, color_, retroStyle_, depth, newMainFrame_);
}

void App::renderStereoOutput(bool newFrame) {
  const GpuImage& v = pipe_.view();
  int w = v.w, h = v.h;
  if (newFrame || !stereoSrc_.tex || stereoSrc_.w != w || stereoSrc_.h != h) {
    // Depth from what is shown (mirror, overlay); the overlay stays flat.
    composeFrame(stereoSrc_, w, h);
    float flat[4];
    stereoDepth_.update(stereoSrc_, overlayRectNorm(flat) ? flat : nullptr);
  }
  if (!stereoDepth_.ready()) return;
  // Largest per-eye shift: 1.4 % of the picture width at full strength.
  float amount = depthStrength_ * 0.014f;
  GlStateGuard guard;
  switch (stereoOut_) {
    case StereoSbsFull:
      stereoImg_.ensure(w * 2, h);
      glBindFramebuffer(GL_FRAMEBUFFER, stereoImg_.fbo);
      stereoDepth_.drawStereo(stereoSrc_, -1, amount, w, h, false, 0, 0);
      stereoDepth_.drawStereo(stereoSrc_, 1, amount, w, h, false, w, 0);
      break;
    case StereoSbsHalf:
      stereoImg_.ensure(w, h);
      glBindFramebuffer(GL_FRAMEBUFFER, stereoImg_.fbo);
      stereoDepth_.drawStereo(stereoSrc_, -1, amount, w / 2, h, false, 0, 0);
      stereoDepth_.drawStereo(stereoSrc_, 1, amount, w - w / 2, h, false, w / 2, 0);
      break;
    case StereoTab:
      stereoImg_.ensure(w, h);
      glBindFramebuffer(GL_FRAMEBUFFER, stereoImg_.fbo);
      stereoDepth_.drawStereo(stereoSrc_, -1, amount, w, h / 2, false, 0, 0);
      stereoDepth_.drawStereo(stereoSrc_, 1, amount, w, h - h / 2, false, 0, h / 2);
      break;
    case StereoAnaglyph:
      stereoL_.ensure(w, h);
      stereoR_.ensure(w, h);
      glBindFramebuffer(GL_FRAMEBUFFER, stereoL_.fbo);
      stereoDepth_.drawStereo(stereoSrc_, -1, amount, w, h, false);
      glBindFramebuffer(GL_FRAMEBUFFER, stereoR_.fbo);
      stereoDepth_.drawStereo(stereoSrc_, 1, amount, w, h, false);
      DepthEngine::anaglyph(stereoL_, stereoR_, stereoImg_);
      break;
    default: return;
  }
  stereoReady_ = true;
}

void App::preRender() {
  newMainFrame_ = false;
  // GPU time of the video work, averaged over frames that brought new video
  // (Settings → About).
  struct TimerScope {
    GpuTimer* t;
    const bool& counted;
    ~TimerScope() {
      if (t) t->end(counted);
    }
  } timerScope{connected_ ? &videoTimer_ : nullptr, newMainFrame_};
  if (connected_) videoTimer_.begin();

  if (connected_) {
    if (cap_.failed()) {
      std::fprintf(stderr, "[uvc-studio] source failed: %s\n", cap_.failure().c_str());
      disconnect();
      setError("err.sourceLost");
      return;
    }
    if (FramePtr f = cap_.latest(lastSeq_)) {
      lastSeq_ = f->seq;
      pipe_.upload(*f);
      newMainFrame_ = true;
    }
    bool newOverlayFrame = false;
    if (overlayCap_.isOpen()) {
      if (overlayCap_.failed()) {
        setError("err.overlayCamera", overlayCap_.failure());
        overlayOn_ = false;
        settings_.setNum("overlay.on", 0);
        closeOverlay();
      } else if (FramePtr f = overlayCap_.latest(lastOverlaySeq_)) {
        lastOverlaySeq_ = f->seq;
        overlayPipe_.upload(*f);
        newOverlayFrame = true;
      }
    }
    if (pipe_.hasImage()) applyEffects();
    // Liquid Glass backdrop (blurred picture) behind the flat interface.
    if (pipe_.hasImage() && !xrMode_ && (newMainFrame_ || !backdrop_.ready)) backdrop_.update(pipe_.view());
    if (overlayPipe_.hasImage()) overlayPipe_.process(Effect::None, ColorAdjust{});
    // Immersive mode: keep the giant-screen picture up to date.
    if (xrMode_ && pipe_.hasImage() && (newMainFrame_ || newOverlayFrame)) {
      const GpuImage& v = pipe_.view();
      int cw = std::min(v.w, vrEffectiveWidth());
      int ch = std::max(2, (int)std::lround((double)cw * v.h / v.w));
      composeFrame(xrScreen_, cw, ch);
      xrScreenSeq_++;
      if (depth3d_) {
        float flat[4];
        xrDepth_.update(xrScreen_, overlayRectNorm(flat) ? flat : nullptr);
      } else if (xrDepth_.ready()) {
        xrDepth_.reset();
      }
    }
    if (stereoActive() && pipe_.hasImage())
      renderStereoOutput(newMainFrame_ || newOverlayFrame || !stereoReady_);
    else
      stereoReady_ = false;

    if (recording_ && pipe_.hasImage()) {
      double t = nowSeconds();
      double interval = 1.0 / std::max(1, recorder_.fps());
      if (newMainFrame_ && t - lastRecFrameT_ >= interval * 0.9) {
        lastRecFrameT_ = t;
        composeFrame(compose_, recW_, recH_);
        if (recorder_.acceptsTexture())
          recorder_.pushTexture(compose_.tex, compose_.w, compose_.h, t);
        else
          readback_.request(compose_, t);
      }
      std::vector<uint8_t> buf;
      int w, h;
      double ft;
      while (readback_.poll(buf, w, h, ft)) recorder_.pushVideo(std::move(buf), w, h, ft);
    }
  }

  // A recording finished: publish it (Quest: copy into the gallery, which
  // can take a few seconds) and say where it went.
  std::string path, err;
  bool ok;
  if (recorder_.pollFinished(path, ok, err)) {
    if (!ok) {
      setError("err.recording", err);
      return;
    }
    std::string saved = i18n::tr(lang_, "toast.savedVideos");
    std::string failed = i18n::tr(lang_, "err.recording");
    ++publishing_;
    std::thread([this, path, saved, failed] {
      std::string where = platform::publishOutput(path, "video/mp4");
      if (where.empty()) {
        std::string msg = failed;
        size_t p = msg.find("{detail}");
        if (p != std::string::npos) msg.replace(p, 8, path);
        pushToast(msg, "", true);
      } else {
        pushToast(saved, where);
      }
      --publishing_;
    }).detach();
  }
}
