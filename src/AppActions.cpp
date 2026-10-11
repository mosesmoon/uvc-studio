// Snapshots, recordings, keyboard and window events.
#include <cmath>
#include <memory>
#include <thread>

#include "App.h"
#include "I18n.h"
#include "Ui.h"
#include "gl.h"
#include "stb_image_write.h"

// ---------------------------------------------------------------------------
// Snapshot: PNG of the composed picture, written in the background.
void App::takeSnapshot() {
  if (!connected_ || !pipe_.hasImage()) return;
  const GpuImage& v = pipe_.view();
  GpuImage shot;
  composeFrame(shot, v.w, v.h);
  auto pixels = std::make_shared<std::vector<uint8_t>>();
  readImage(shot, *pixels);
  int w = shot.w, h = shot.h;
  shot.release();
  std::string path = platform::newOutputPath(platform::picturesDir(), ".png");
  std::string saved = i18n::tr(lang_, "toast.savedPictures");
  const bool canShow = true;
  std::string failed = i18n::tr(lang_, "err.snapshot");
  ++publishing_;
  std::thread([this, pixels, w, h, path, saved, failed, canShow] {
    stbi_write_png_compression_level = 3;
    bool written = stbi_write_png(path.c_str(), w, h, 4, pixels->data(), w * 4) != 0;
    std::string where = written ? platform::publishOutput(path, "image/png") : "";
    if (!where.empty()) {
      pushToast(saved, canShow ? where : "");
    } else {
      std::string msg = failed;
      size_t p = msg.find("{detail}");
      if (p != std::string::npos) msg.replace(p, 8, path);
      pushToast(msg, "", true);
    }
    --publishing_;
  }).detach();
}

// ---------------------------------------------------------------------------
// Recording: the composed picture plus game audio (and the mic when on).
void App::startRecording() {
  if (!connected_ || !pipe_.hasImage() || recording_) return;
  if (recorder_.busy()) {
    // The previous file is still being finalised.
    std::string p, e;
    bool ok;
    if (!recorder_.pollFinished(p, ok, e)) return;
  }
  const GpuImage& v = pipe_.view();
  Recorder::Options o;
  o.width = v.w & ~1;
  o.height = v.h & ~1;
  double rate = streamActual_.fps > 0 ? std::min<double>(streamActual_.fps, modeFps_) : modeFps_;
  o.fps = std::max(1, (int)std::lround(rate));
  o.videoBitrate = recordingBitrate(o.width, o.height, o.fps);
  o.gameAudio = audio_.passthroughRunning();
  o.mic = micRec_;
  o.path = platform::newOutputPath(platform::videosDir(), ".mp4");
  std::string err;
  if (!recorder_.start(o, &audio_, err)) {
    if (err.rfind("No video encoder", 0) == 0)
      setError("err.noEncoder");
    else
      setError("err.recording", err);
    return;
  }
  recW_ = o.width;
  recH_ = o.height;
  readback_.reset();
  lastRecFrameT_ = 0;
  recording_ = true;
  recStart_ = nowSeconds();
  std::printf("[uvc-studio] recording %dx%d@%d %.1f Mbit/s with %s to %s\n", o.width, o.height, o.fps,
              o.videoBitrate / 1e6, recorder_.encoderName().c_str(), o.path.c_str());
}

// The file is finalised on the recorder thread; preRender() announces it.
void App::stopRecording() {
  if (!recording_) return;
  std::vector<uint8_t> buf;
  int w, h;
  double t;
  while (readback_.poll(buf, w, h, t, true)) recorder_.pushVideo(std::move(buf), w, h, t);
  readback_.reset();
  recorder_.stop();
  recording_ = false;
}

void App::openSnapshotsFolder() { platform::openFolder(platform::picturesDir()); }

void App::openRecordingsFolder() { platform::openFolder(platform::videosDir()); }

// ---------------------------------------------------------------------------
// Keyboard (§8).
void App::handleKey(const SDL_KeyboardEvent& e) {
  const SDL_Keysym& k = e.keysym;
  // The shortcut sheet closes on any key.
  if (shortcutsOpen_) {
    shortcutsOpen_ = false;
    return;
  }
  // Esc cancels a Settings card drag (the card flies back); other keys wait.
  if (!cardDrag_.empty()) {
    if (k.sym == SDLK_ESCAPE) cancelCardDrag();
    return;
  }
  // Keys go to a focused text field or open menu, not to UVC Studio.
  if (ImGui::GetIO().WantTextInput || anyComboOpen_ || langOpen_) {
    if (k.sym == SDLK_ESCAPE) langOpen_ = false;
    return;
  }
  if (k.mod & KMOD_ALT) return;
  bool ctrl = (k.mod & (KMOD_CTRL | KMOD_GUI)) != 0;
  bool shift = (k.mod & KMOD_SHIFT) != 0;

  if (ctrl) {
    switch (k.sym) {
      case SDLK_EQUALS:
      case SDLK_PLUS:
      case SDLK_KP_PLUS: zoom(uiZoom_ + 0.1f); break;
      case SDLK_MINUS:
      case SDLK_KP_MINUS: zoom(uiZoom_ - 0.1f); break;
      case SDLK_0:
      case SDLK_KP_0: zoom(1.f); break;
      case SDLK_d:
        if (!e.repeat) disconnect();
        break;
      case SDLK_COMMA:
        if (!e.repeat) settingsOpen_ = !settingsOpen_;
        break;
      default: break;
    }
    return;
  }
  if (e.repeat) return;

  switch (k.sym) {
    case SDLK_ESCAPE:
      if (aboutOpen_)
        aboutOpen_ = false;
      else if (fullscreen_)
        toggleFullscreen();
      else if (settingsOpen_)
        settingsOpen_ = false;
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      if (!connected_) connect();
      break;
    case SDLK_s: takeSnapshot(); break;
    case SDLK_r: toggleRecording(); break;
    case SDLK_m: setMicRecording(!micRec_); break;
    case SDLK_l: setListen(!listen_); break;
    case SDLK_w: toggleOverlay(); break;
    case SDLK_x: toggleMirror(); break;
    case SDLK_u:
      if (shift) {
        setUpscaleMode((upscaleMode_ + 1) % kUpscaleModes);
        setUpscale(true);
      } else {
        toggleUpscale();
      }
      break;
    case SDLK_c: cycleRetro(); break;
    case SDLK_d: toggleDepth(); break;
    case SDLK_o: settingsOpen_ = !settingsOpen_; break;
    case SDLK_f:
    case SDLK_F11: toggleFullscreen(); break;
    case SDLK_h:
    case SDLK_QUESTION: shortcutsOpen_ = true; break;
    case SDLK_SLASH:
      if (shift) shortcutsOpen_ = true;
      break;
    default: break;
  }
}

void App::onEvent(const SDL_Event& e) {
  switch (e.type) {
    case SDL_QUIT: quit_ = true; break;
    case SDL_WINDOWEVENT:
      if (e.window.event == SDL_WINDOWEVENT_CLOSE && win_ && e.window.windowID == SDL_GetWindowID(win_)) quit_ = true;
      break;
    case SDL_MOUSEBUTTONDOWN:
      // A click anywhere closes the shortcut sheet.
      shortcutsOpen_ = false;
      break;
    case SDL_KEYDOWN: handleKey(e.key); break;
    default: break;
  }
}
