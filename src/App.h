// UVC Studio — application state, behaviour (App.cpp, AppActions.cpp) and
// interface drawing (AppUi.cpp).
#pragma once
#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "AudioEngine.h"
#include "Depth3D.h"
#include "Platform.h"
#include "Recorder.h"
#include "VideoCapture.h"
#include "VideoRenderer.h"
#include "imgui.h"
// Quest editions (free daily allowance, UVC Studio Platinum): always on for
// Quest; a desktop build can turn the interface on to test it.

class App {
 public:
  bool init(SDL_Window* win);
  void shutdown();
  // One UI frame (between ImGui::NewFrame and ImGui::Render).
  void frame();
  // GPU work that must happen before the UI draws (uploads, picture chain,
  // recording composition).
  void preRender();
  void onEvent(const SDL_Event& e);
  bool quitRequested() const { return quit_; }

  // ---- Quest host hooks (2D window and immersive mode) ----
  void setXrMode(bool on) { xrMode_ = on; }
  bool connected() const { return connected_; }
  // Connects again after the 2D ⇄ VR switch (the other activity calls this
  // when the previous one was connected).
  void reconnect() { connect(); }
  // Composed picture for the VR screen, and a counter that increments
  // whenever it changes.
  const GpuImage& xrScreen() const { return xrScreen_; }
  uint64_t xrScreenSeq() const { return xrScreenSeq_; }
  bool panelHidden() const { return xrMode_ && fullscreen_; }
  void togglePanel() { toggleFullscreen(); }
  bool vrPassthrough() const { return vrPassthrough_; }
  // The overheat guard may cap it (72 Hz) while the headset is hot.
  int vrRefreshRate() const { return refreshCap_ ? std::min(vrRefresh_, refreshCap_) : vrRefresh_; }
  // Pixels across the giant screen worth rendering (~30 px per degree of its
  // angular width, capped at 2560). Upscaling and the stereo pair stop there.
  int vrEffectiveWidth() const;
  // OpenXR thermal notification level (0 normal, 25 warning, 75 impaired).
  void setXrThermalLevel(int level) { xrThermal_ = level; }
  GpuTimer& stereoTimer() { return stereoTimer_; }
  // The runtime took input focus away (system menu): mute and wait.
  void setXrFocused(bool f) { xrFocused_ = f; }
  // 3D depth is on (any format but "No depth"); in immersive mode that means
  // the stereo giant screen.
  bool depth3D() const { return depth3d_ && connected_; }
  float depthStrength() const { return depthStrength_; }
  DepthEngine& xrDepth() { return xrDepth_; }
  float vrScreenWidth() const { return vrScreenWidth_; }
  float vrScreenDistance() const { return vrScreenDist_; }
  void setVrScreen(float width, float dist, bool persist);
  // Immersive mode: where the control panel floats, relative to the user —
  // yaw (radians, + = left), height of its centre (m, relative to the eyes)
  // and distance (m). One place while connected (below the screen), one
  // while idle (in front). Moved with the bar under the panel.
  struct VrPlace {
    float yaw = 0, height = 0, dist = 1;
  };
  static VrPlace defaultVrPanel(bool live) { return live ? VrPlace{0, -0.45f, 0.85f} : VrPlace{0, -0.12f, 1.0f}; }
  VrPlace vrPanelPlace(bool live) const { return vrPanel_[live ? 1 : 0]; }
  void setVrPanelPlace(bool live, VrPlace p, bool persist);
  bool vrPanelMoved() const;
  void resetVrPanel();
  bool vrCurved() const { return vrCurved_; }
  // Set when the 2D ⇄ VR switch was pressed; read once by the host.
  bool takeModeSwitchRequest() {
    bool r = requestModeSwitch_;
    requestModeSwitch_ = false;
    return r;
  }

  // ---- overlay geometry (also used by the interface) ----
  enum OverlayCorner { CornerTopLeft = 0, CornerTopRight, CornerBottomLeft, CornerBottomRight };
  enum OverlaySize { OverlaySmall = 0, OverlayMedium, OverlayLarge };
  // Overlay rectangle {x0, y0, x1, y1} in normalised video coordinates (0…1,
  // top-left origin) for a video of aspect `videoAspect` (width / height):
  // width = 18 / 26 / 36 % of the video width, 16:9, 3 % margin (of the video
  // width) from the two edges of `corner`.
  static void overlayRectFor(int corner, int size, float videoAspect, float r[4]);
  // Corner closest to a point in normalised video coordinates (e.g. the
  // centre of a dragged overlay).
  static int nearestCorner(float nx, float ny);
  // "720p · 30 fps", "1080p · 30 fps", "1080p · 60 fps".
  static constexpr int kOverlayQualities = 3;
  static const char* overlayQualityLabel(int q);

  // ---- video mode labels ----
  static std::string resolutionText(int w, int h);  // "1920 × 1080"
  static std::string resolutionTag(int w, int h);   // "Full HD", "" when none
  static std::string resolutionLabel(int w, int h); // "1920 × 1080 · Full HD"
  static std::string fpsLabel(int fps);             // "60 fps"

  // ---- picture presets (Settings → Picture chips) ----
  struct PicturePreset {
    const char* key;                       // i18n key of the chip label
    int brightness, contrast, saturation;  // −40 … +40
    float gamma;                           // 0.70 … 1.40
  };
  static constexpr int kPicturePresets = 5;  // Natural, Realism, Punchy, Film, Bright room
  static const PicturePreset& picturePreset(int i);

  // Recording video bitrate: w × h × fps × 0.12 bits/s within 8 … 50 Mbit/s.
  static int64_t recordingBitrate(int w, int h, int fps);

 private:
  // =========================================================================
  // Session (§2)
  // =========================================================================
  bool connected_ = false;
  bool autoConnect_ = true;
  // Opens the source video, game audio, the overlay camera (if on) and the
  // mic (if mic recording is on). No-op when connected.
  void connect();
  // Stops recording (finalised and announced), then closes everything.
  void disconnect();
  void setAutoConnect(bool on);
  // A source device exists (the Connect button can be pressed).
  bool canConnect() const { return currentVideoDev() != nullptr; }

  // ---- errors (§10): an i18n key plus an optional {detail} ----
  std::string errKey_, errDetail_;
  double errAt_ = 0;
  static constexpr double kErrorSeconds = 6.0;
  void setError(const char* key, const std::string& detail = "");
  void clearError();
  // Translated message, "" when there is none or it is older than 6 s.
  std::string errorText() const;

  // =========================================================================
  // Devices (§3)
  // =========================================================================
  std::vector<VideoDeviceInfo> videoDevs_;
  std::vector<AudioDeviceInfo> audioIns_, audioOuts_;
  // Saved choices ("" = automatic) and the devices actually in use.
  std::string savedVideo_, selVideo_;
  std::string savedAudioIn_, selAudioIn_;
  std::string selAudioOut_ = "default";
  std::string micSource_;  // "" = default microphone
  // Video devices that can be opened (listed with modes), for auto-connect.
  std::set<std::string> openableVideo_;
  bool firstDeviceList_ = true;
  // Takes the newest device list from the watcher, re-picks automatic
  // choices and auto-connects when a likely capture card appeared.
  void applyDeviceList();
  void pickSource();
  void pickGameAudio();
  void pickOverlayCamera();
  static int sourceScore(const VideoDeviceInfo& d);
  static int overlayScore(const VideoDeviceInfo& d);
  const VideoDeviceInfo* findVideo(const std::string& id) const;
  const VideoDeviceInfo* currentVideoDev() const { return findVideo(selVideo_); }
  std::string currentVideoLabel() const;
  std::string currentAudioLabel() const;
  // Explicit changes from the interface (persisted; reopen what is needed).
  void setSourceDevice(const std::string& id);  // "" = automatic
  void setGameAudioInput(const std::string& id);  // "" = automatic
  void setAudioOutput(const std::string& id);   // "default" = system default
  void setMicSource(const std::string& id);

  // =========================================================================
  // Video mode (§4)
  // =========================================================================
  // Saved mode (0 = none) and the mode in use for the current source.
  int savedW_ = 0, savedH_ = 0, savedFps_ = 0;
  int modeW_ = 1920, modeH_ = 1080, modeFps_ = 60;
  uint32_t formatPref_ = PREFER_AUTO;
  StreamSettings streamActual_;
  // Resolutions the source reports, largest first; frame rates at one of
  // them, highest first (rounded).
  std::vector<std::pair<int, int>> sourceResolutions() const;
  std::vector<int> sourceFrameRates(int w, int h) const;
  // The saved mode when the source offers it, otherwise the default.
  void resolveMode();
  void setSourceResolution(int w, int h);
  void setSourceFrameRate(int fps);
  void setSourceFormat(uint32_t pref);
  int resW() const { return modeW_; }
  int resH() const { return modeH_; }

  // =========================================================================
  // Audio
  // =========================================================================
  bool listen_ = true;   // play game audio through the output (Listen)
  float volume_ = 1.f;   // listen volume 0 … 1
  bool micRec_ = false;  // record the microphone
  // Quest: the card's audio channels — 2 (stereo) or 6 (5.1 surround).
  int gameChannels_ = 2;
  void setGameChannels(int n);
  void setListen(bool on);
  void setListenVolume(float v);
  void setMicRecording(bool on);
  void applyVolume();
  // Game audio capture (always while connected) and monitoring.
  void startGameAudio();
  void restartAudio();
  void startMic();

  // =========================================================================
  // Webcam overlay (§5)
  // =========================================================================
  bool overlayOn_ = false;
  std::string savedOverlayCam_, overlayCam_;
  int overlayQuality_ = 0;
  int overlayCorner_ = CornerBottomLeft;
  int overlaySize_ = OverlayMedium;
  bool overlayMirror_ = false;
  bool overlayFailed_ = false;  // error shown once for this attempt
  // Temporary rectangle while the interface drags / animates the overlay.
  bool overlayHeld_ = false;
  float overlayHeldRect_[4] = {0, 0, 0, 0};
  // Rectangle used by the screen, recordings, snapshots, depth and VR.
  // False when no overlay picture is shown.
  bool overlayRectNorm(float r[4]) const;
  // Interface drag / animation: r = rectangle to use instead of the corner
  // placement; nullptr returns to the corner placement.
  void setOverlayDisplayRect(const float* r);
  void setOverlayOn(bool on);
  void toggleOverlay() { setOverlayOn(!overlayOn_); }
  void setOverlayCorner(int corner);
  void setOverlaySize(int size);
  void cycleOverlaySize() { setOverlaySize((overlaySize_ + 1) % 3); }
  void stepOverlaySize(int dir);  // mouse wheel: +1 larger, −1 smaller
  void setOverlayMirror(bool on);
  void setOverlayCamera(const std::string& id);  // "" = automatic
  void setOverlayQuality(int q);
  bool canSwapPictures() const;
  // The overlay camera becomes the source and the source the overlay.
  void swapPictures();
  void openOverlay();
  void closeOverlay();

  // =========================================================================
  // Picture (§6)
  // =========================================================================
  ColorAdjust color_;  // gains 0.60 … 1.40, gamma 0.70 … 1.40
  bool mirror_ = false;
  bool upscale_ = false;
  static constexpr int kUpscaleModes = 4;  // FSR, Anime4K, Lanczos + CAS, NIS
  int upscaleMode_ = 0;
  // Retro look (RetroStyle): the one shown (RetroOff = none, saved as
  // picture.retro) and the last one shown (the dock tile turns it on again).
  int retroStyle_ = RetroOff;
  int retroLast_ = RetroCrt;
  static constexpr int kRetroStyles = 3;  // CRT, VHS, Arcade
  enum PictureControl { PicBrightness = 0, PicContrast, PicSaturation };
  int pictureValue(int which) const;  // −40 … +40
  void setPictureValue(int which, int v);
  static constexpr float kGammaMin = 0.70f, kGammaMax = 1.40f;
  void setGamma(float g);  // kGammaMin … kGammaMax, rounded to 0.01
  void applyPicturePreset(int i);
  int activePicturePreset() const;  // −1 when none matches
  void toggleMirror();
  void toggleUpscale();
  void setUpscale(bool on);  // the mode is kept for next time
  void setUpscaleMode(int m);
  // Retro look: RetroOff or a RetroStyle (remembered as the last one).
  void setRetroStyle(int s);
  void toggleRetro();  // dock tile: off ↔ the last style
  void cycleRetro();   // C: off → CRT → VHS → Arcade → off
  // 3D depth format (depth.format). Desktop: all; Quest: none / effect
  // (immersive mode shows "effect" as the stereo giant screen).
  enum DepthFormat { DepthNone = 0, DepthEffect, DepthSbsFull, DepthSbsHalf, DepthTab, DepthAnaglyph };
  int depthFormat() const { return depth3d_ ? DepthEffect + stereoOut_ : DepthNone; }
  int depthLast_ = DepthEffect;  // format D / the tile turns on again
  void setDepthFormat(int f);
  void toggleDepth();  // D, dock tile: "No depth" ↔ the last format
  void setDepthStrength(float v);

  // =========================================================================
  // Snapshots, recordings (§7)
  // =========================================================================
  bool recording_ = false;
  double recStart_ = 0;
  void takeSnapshot();
  void startRecording();
  void stopRecording();
  void toggleRecording() { recording_ ? stopRecording() : startRecording(); }
  void openSnapshotsFolder();
  void openRecordingsFolder();

  // =========================================================================
  // Keyboard (§8)
  // =========================================================================
  bool shortcutsOpen_ = false;  // shortcut sheet shown
  void handleKey(const SDL_KeyboardEvent& k);

  // ---- interface state ----
  std::string lang_;
  bool settingsOpen_ = false;
  bool langOpen_ = false;
  ImVec2 langBtnMin_, langBtnMax_, langMenuMin_, langMenuMax_;
  bool aboutOpen_ = false;
  bool fullscreen_ = false;
  bool xrMode_ = false;
  bool xrFocused_ = true;
  int vrRefresh_ = 90;
  bool depth3d_ = false;
  float depthStrength_ = 0.6f;
  DepthEngine xrDepth_;
  // Stereo output of the desktop depth formats (StereoOff = the 2D depth
  // effect); depthFormat() = DepthEffect + stereoOut_ while depth is on.
  enum StereoOut { StereoOff = 0, StereoSbsFull, StereoSbsHalf, StereoTab, StereoAnaglyph };
  int stereoOut_ = StereoOff;
  DepthEngine stereoDepth_;
  GpuImage stereoSrc_, stereoImg_, stereoL_, stereoR_;
  bool stereoReady_ = false;
  bool stereoActive() const { return !xrMode_ && depth3d_ && stereoOut_ != StereoOff && connected_; }
  void renderStereoOutput(bool newFrame);
  std::string themePref_ = "auto";  // auto | dark | light
  float themeMix_ = -1.f;
  float appliedTheme_ = -1.f;
  bool vrPassthrough_ = true;
  float vrScreenWidth_ = 2.6f;  // metres
  float vrScreenDist_ = 2.2f;   // metres
  VrPlace vrPanel_[2] = {defaultVrPanel(false), defaultVrPanel(true)};  // idle, live
  bool vrCurved_ = false;
  bool requestModeSwitch_ = false;
  GpuImage xrScreen_;
  uint64_t xrScreenSeq_ = 0;
  // Tooltip of the hovered control: name, shortcut key(s) shown as keycaps
  // ("Ctrl+D"), and whether it opens below the anchor (top-bar controls).
  std::string tooltip_, tooltipKey_;
  ImVec2 tooltipAnchor_;
  bool tooltipBelow_ = false;

  // ---- Settings card arrangement (spec §9.6) ----
  // The user's three-column arrangement of the Settings cards (card ids, top
  // to bottom; settings key ui.settingsLayout). All empty = the automatic
  // balanced layout. Unlocked cards (ui.settingsUnlocked) can be dragged by
  // their header; every card starts locked.
  std::vector<std::string> cardCols_[3];
  std::set<std::string> cardsUnlocked_;
  bool customCardLayout() const;
  void loadCardLayout();
  void saveCardLayout();
  void resetCardLayout();  // automatic layout, every card locked
  void setCardUnlocked(const std::string& id, bool unlocked);
  // Collapsed cards (ui.settingsFolded) show only their header; the fold
  // button right of the padlock toggles it, animated over ~220 ms.
  std::set<std::string> cardsFolded_;
  std::map<std::string, double> cardFoldT0_;
  void setCardFolded(const std::string& id, bool folded);
  // Moves card `id` to column `col` before the visible card at `visIdx`
  // (`visible` = the cards shown now, top to bottom per column).
  void moveCard(const std::string& id, int col, int visIdx, const std::vector<std::vector<std::string>>& visible);
  // Header press (waiting for a few pixels of movement), then the drag.
  std::string cardPress_, cardDrag_;
  ImVec2 cardPressPos_, cardGrab_;
  ImVec2 cardGhostRel_;  // ghost's top-left relative to the grid origin
  double cardDragT0_ = 0;
  void cancelCardDrag();
  // Card positions (relative to the grid origin) ease to their slots over
  // 150 ms while a drag is in progress and just after a drop / reset.
  struct CardAnim {
    ImVec2 from, to, cur;
    double t0 = 0;
  };
  std::map<std::string, CardAnim> cardAnim_;
  double cardAnimUntil_ = 0;

  // ---- engine ----
  SDL_Window* win_ = nullptr;
  Settings settings_;
  AudioEngine audio_;
  VideoCapture cap_;
  VideoCapture overlayCap_;
  VideoPipeline pipe_;
  VideoPipeline overlayPipe_;
  uint64_t lastSeq_ = 0, lastOverlaySeq_ = 0;
  bool newMainFrame_ = false;
  GpuTimer videoTimer_, stereoTimer_;
  int refreshCap_ = 0;
  int thermalLevel_ = 0;     // 0 normal, 1 warm, 2 hot
  int thermalActed_ = 0;
  int thermalStatus_ = -1;
  float thermalHeadroom_ = -1;
  int xrThermal_ = 0;
  double cooledSince_ = 0;
  double streamStartT_ = 0;
  int stallRetries_ = 0;
  bool stallReported_ = false;
  static constexpr const char* kUsbCardAudio = "usb:capture-card";
  Recorder recorder_;
  Readback readback_;
  GpuImage compose_;
  double lastRecFrameT_ = 0;
  int recW_ = 0, recH_ = 0;

  // ---- device watcher (background thread) ----
  std::thread watcher_;
  std::atomic<bool> watcherStop_{false};
  std::mutex devMu_;
  std::vector<VideoDeviceInfo> pendingVideo_;
  std::vector<AudioDeviceInfo> pendingIns_, pendingOuts_;
  std::atomic<int> devVersion_{0};
  int appliedVersion_ = 0;
  std::atomic<bool> refreshNow_{false};
  void startWatcher();

  // ---- interface layout cache ----
  ImVec2 stageMin_, stageMax_;
  ImVec2 stageRefMin_, stageRefMax_;  // stage of the regular (non-fullscreen) layout
  std::vector<std::vector<std::string>> licenseLines_;
  std::vector<float> licenseWrapW_;
  float fbScale_ = 1.f;
  bool quit_ = false;
  struct Toast {
    std::string text;
    std::string path;  // "Show" reveals it ("" = no action)
    double until;
    bool error;
  };
  std::vector<Toast> toasts_;
  std::mutex toastMu_;
  std::atomic<int> publishing_{0};  // background snapshot / gallery copies
  std::vector<bool> licenseOpen_;
  std::vector<std::string> licenseText_;
  double listenHoverUntil_ = 0, overlayHoverUntil_ = 0, depthHoverUntil_ = 0, upscaleHoverUntil_ = 0,
         retroHoverUntil_ = 0;
  bool anyComboOpen_ = false;
  ImVec2 settingsAnchor_, listenAnchor_, overlayAnchor_, depthAnchor_, upscaleAnchor_, retroAnchor_, settingsRectMin_,
      settingsRectMax_;
  bool listenBtnHover_ = false, overlayBtnHover_ = false, depthBtnHover_ = false, upscaleBtnHover_ = false,
       retroBtnHover_ = false;
  // Top of the dock tiles (popover windows stay above it so a click on a
  // tile always reaches the tile).
  float dockTileTop_ = 0;
  float uiZoom_ = 1.f;
  int uiScalePct_ = 0;  // 0 = automatic
  float autoScale_ = 1.f;
  float appliedScale_ = -1.f;

  // Per-frame engine housekeeping (device list, Quest permissions, volume).
  void tick();
  void openSource();   // opens the source video (and Quest card audio)
  void reopenVideo();  // video stream only; audio keeps running
  void closeAll();
  void updateTheme();
  void toggleFullscreen();
  void applyEffects();
  void composeFrame(GpuImage& target, int w, int h);
  void pushToast(const std::string& text, const std::string& path, bool error = false);
  void setLang(const std::string& l);
  void zoom(float z);

  // ---- drawing (AppUi.cpp) ----
  void drawTopbar(ImDrawList* dl, ImVec2 a, ImVec2 b);
  void drawStage(ImDrawList* dl, ImVec2 a, ImVec2 b);
  void drawIdle(ImDrawList* dl, ImVec2 a, ImVec2 b);
  void drawOverlay(ImDrawList* dl);
  void drawDock(ImDrawList* dl, ImVec2 a, ImVec2 b);
  void drawSettingsPopover(ImVec2 anchor);
  void drawLangMenu();
  void drawListenPopover(ImVec2 anchorCenterTop, bool& hovered);
  void drawOverlayPopover(ImVec2 anchorCenterTop, bool& hovered);
  void drawDepthPopover(ImVec2 anchorCenterTop, bool& hovered);
  void drawUpscalePopover(ImVec2 anchorCenterTop, bool& hovered);
  void drawRetroPopover(ImVec2 anchorCenterTop, bool& hovered);
  // 3D depth format choices for this mode (popover and Settings): value
  // (DepthFormat) and label.
  std::vector<std::pair<int, std::string>> depthFormatOptions() const;
  void drawAbout();
  void drawPopovers();
  void drawTooltip(ImDrawList* dl);
  void drawShortcutSheet(ImDrawList* dl);
  void drawErrorLine(ImDrawList* dl, float centerX, float top);
  void drawToasts(ImDrawList* dl);
  struct DockItem {
    const char* id = "";
    const char* icon = "";
    std::string label;
    const char* shortcut = "";
    bool active = false, disabled = false, redActive = false, sepBefore = false;
    std::function<void()> onClick;
    ImVec2* anchor = nullptr;
    bool* hoverOut = nullptr;
    ImVec2 baseMin, baseMax;
  };
  void drawTiles(ImDrawList* dl, std::vector<DockItem>& items, float x0, float baseBottom, float tile, float gap);
  float tilesWidth(const std::vector<DockItem>& items, float tile, float gap) const;
  float drawStatus(ImDrawList* dl, float x, float cy, float maxW, bool draw);
  std::map<std::string, float> tileScale_;
  std::map<std::string, double> tileBounce_;
  Backdrop backdrop_;
  // Where the picture is drawn on screen (overlay coordinates map onto it).
  ImVec2 picMin_, picMax_;
  // Overlay drag state (interface): while dragging, and for the 150 ms
  // ease-out back to the nearest corner after release.
  bool overlayDragging_ = false;
  ImVec2 overlayGrab_;
  bool overlaySnapping_ = false;
  float overlaySnapFrom_[4] = {0, 0, 0, 0};
  double overlaySnapStart_ = 0;
  double overlayChromeUntil_ = 0;  // hover toolbar stays briefly after leaving
  // App icon on the ready screen (a texture of this GL context).
  unsigned appIconTex_ = 0;
  bool appIconTried_ = false;
};
