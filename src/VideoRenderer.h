// GPU side of the video path: colour conversion of captured frames, the
// picture chain (picture controls, 4K upscaling, TV style) and composition of
// the recording / snapshot canvas.
//
// 4K upscaling comes in four flavours (all spatial, frame by frame):
//   - FSR: AMD FidelityFX Super Resolution 1 (EASU edge-adaptive upscale +
//     RCAS sharpening), a spatial upscaler that suits any content;
//   - Anime4K: bloc97's CNN x2 line-art network (M on desktop, S on Quest),
//     tuned for animation, then resampled to the 4K target;
//   - Lanczos + CAS: Lanczos-3 resampling, then AMD FidelityFX CAS;
//   - NIS: NVIDIA Image Scaling (directional 6-tap scaler + adaptive USM).
#pragma once
#include <cstdint>
#include <deque>
#include <vector>

#include "Frame.h"
#include "gl.h"

struct GpuImage {
  GLuint tex = 0;
  GLuint fbo = 0;
  int w = 0;
  int h = 0;
  GLenum fmt = 0;
  // (Re)allocates at w × h. ifmt GL_RGBA16F for signed intermediate data.
  void ensure(int w, int h, GLenum ifmt = GL_RGBA8);
  void release();
};

// Upscaler choice (None = picture shown at its own size).
enum class Effect { None, Upscale, Anime4K, LanczosCas, Nis };

inline bool isUpscale(Effect e) {
  return e == Effect::Upscale || e == Effect::Anime4K || e == Effect::LanczosCas || e == Effect::Nis;
}

// Picture controls as gains: 1.0 neutral, 0.60 … 1.40 (shown as −40 … +40).
// Gamma is applied last as c^(1 / gamma), so values above 1 brighten the
// mid-tones.
struct ColorAdjust {
  float brightness = 1.f;
  float contrast = 1.f;
  float saturation = 1.f;
  float gamma = 1.f;
  bool identity() const { return brightness == 1.f && contrast == 1.f && saturation == 1.f && gamma == 1.f; }
  bool operator==(const ColorAdjust& o) const {
    return brightness == o.brightness && contrast == o.contrast && saturation == o.saturation && gamma == o.gamma;
  }
};

// TV style drawn over the finished picture (after any upscaler).
enum RetroStyle { RetroOff = 0, RetroCrt, RetroVhs, RetroArcade };

// Compiles the shared vertex shader with `fragmentSrc` (GLSL 330 core; turned
// into GLSL ES 300 on Quest). Returns 0 on failure (logged).
GLuint linkProgram(const char* fragmentSrc);

// Saves / restores the GL state our offscreen passes touch.
struct GlStateGuard {
  GlStateGuard();
  ~GlStateGuard();
  GlStateGuard(const GlStateGuard&) = delete;
  GlStateGuard& operator=(const GlStateGuard&) = delete;
  GLint fbo = 0, prog = 0, vao = 0, tex = 0, active = 0, viewport[4] = {0, 0, 0, 0};
  GLboolean blend = 0, scissor = 0, depth = 0, cull = 0;
};

// Shared GL programs. Create once after the GL context exists.
class GpuPrograms {
 public:
  bool init();
  void shutdown();
  static GpuPrograms& get();

  GLuint convert = 0, bcs = 0, easu = 0, rcas = 0, lanczos = 0, cas = 0, nis = 0, crt = 0, vhs = 0, arcade = 0,
         blit = 0, blur = 0;
  GLuint nisCoef = 0;  // NIS filter banks (4 × 64 RGBA32F)
  // Anime4K network passes, compiled on first use. Empty = unsupported.
  const std::vector<GLuint>& anime4k();
  int anime4kState = 0;  // 0 not tried, 1 ready, -1 unavailable
  std::vector<GLuint> a4k;
  GLuint vao = 0;
  // Draws a quad covering `dst` (x0,y0,x1,y1 in pixels of the bound target,
  // top-down) sampling `uv` (u0,v0,u1,v1) with the currently bound program.
  void quad(GLuint prog, int targetW, int targetH, float x0, float y0, float x1, float y1, float u0 = 0,
            float v0 = 0, float u1 = 1, float v1 = 1);
};

// One per live source (main capture, webcam overlay).
class DepthEngine;
struct Anime4KTextures;

class VideoPipeline {
 public:
  ~VideoPipeline();
  // Uploads + converts a frame into `src()`.
  void upload(const Frame& f);
  // Runs the picture chain into `view()`: upscaler (with the picture
  // controls in its last pass), then the TV style `retro` (RetroStyle: CRT,
  // VHS or Arcade; 0 = none). depth > 0 adds the 3D depth enhancer at that
  // strength (0 … 1); `newFrame` refreshes its depth map.
  void process(Effect upscaler, const ColorAdjust& c, int retro = RetroOff, float depth = 0.f, bool newFrame = true);
  // Size (pixels) the picture is shown at; the TV-style patterns are laid out
  // for it so they do not alias when the image is scaled for display.
  void setDisplaySize(int w, int h) { dispW_ = w, dispH_ = h; }
  // Clock (seconds) for the animated VHS noise and tracking; read on frames
  // that are processed.
  void setTime(double t) { time_ = t; }
  const GpuImage& src() const { return src_; }
  const GpuImage& view() const { return *viewPtr_; }
  // Output size of the upscalers for a w × h source: scale = min(maxW / w,
  // maxH / h), at most 4×, and the source size when that is below 1.05×
  // (3840 × 2160 = 4K; Quest immersive mode passes what the giant screen can
  // actually show).
  static void upscaleSize(int w, int h, int& ow, int& oh, int maxW = 3840, int maxH = 2160);
  void setUpscaleLimit(int maxW, int maxH) { limW_ = maxW, limH_ = maxH; }
  bool hasImage() const { return src_.tex != 0 && src_.w > 0; }
  void release();

 private:
  void runEffect(Effect upscaler, const ColorAdjust& c);
  void runRetro(int style, const GpuImage& in);
  void runFsr(const GpuImage& in, int W, int H, const ColorAdjust& c);
  bool runAnime4K(int W, int H, const ColorAdjust& c);
  void runLanczosCas(int W, int H, const ColorAdjust& c);
  void runNis(int W, int H, const ColorAdjust& c);
  // Lanczos-3 resample of `in` to W × H into `out` (two passes via lzTmp_).
  void lanczos(const GpuImage& in, int W, int H, GpuImage& out);
  void runColor(const GpuImage& in, int W, int H, const ColorAdjust& c);
  GLuint planes_[3] = {0, 0, 0};
  int planeW_[3] = {0, 0, 0}, planeH_[3] = {0, 0, 0};
  GLenum planeFmt_[3] = {0, 0, 0};
  GpuImage src_;
  GpuImage fx_;
  GpuImage easu_;
  GpuImage a4kOut_;
  GpuImage lzTmp_, lzOut_, nisOut_;
  GpuImage crtOut_;  // TV-style output
  Anime4KTextures* a4kTex_ = nullptr;
  GpuImage depthFx_;
  // Effect output of the last runEffect(); reused while no new frame arrives.
  const GpuImage* fxOut_ = &src_;
  struct Key {
    Effect e = Effect::None;
    int retro = RetroOff;
    ColorAdjust c;
    int w = 0, h = 0, dw = 0, dh = 0, lw = 0, lh = 0;
    GLuint tex = 0;
    bool operator==(const Key& o) const {
      return e == o.e && retro == o.retro && c == o.c && w == o.w && h == o.h && dw == o.dw && dh == o.dh &&
             lw == o.lw && lh == o.lh && tex == o.tex;
    }
  };
  Key lastKey_;
  bool haveFx_ = false;
  DepthEngine* depth_ = nullptr;
  int dispW_ = 0, dispH_ = 0;
  double time_ = 0;
  int limW_ = 3840, limH_ = 2160;
  const GpuImage* viewPtr_ = &src_;
};

// Asynchronous GPU → CPU readback of RGBA images via pixel buffer objects.
class Readback {
 public:
  ~Readback();
  void request(const GpuImage& img, double timestamp);
  // Returns true and fills `out` when an older request has completed.
  bool poll(std::vector<uint8_t>& out, int& w, int& h, double& timestamp, bool wait = false);
  void reset();

 private:
  struct Slot {
    GLuint pbo = 0;
    GLsync fence = nullptr;
    int w = 0, h = 0;
    size_t size = 0;
    double t = 0;
    bool pending = false;
  };
  Slot slots_[4];
  int next_ = 0;
  std::deque<int> order_;
};

// GPU time of a block of work (timer queries, read back a few frames later,
// never stalling). Settings → About shows it.
class GpuTimer {
 public:
  ~GpuTimer();
  void begin();
  // `count`: include this sample in the average (e.g. only frames that had
  // new video to process).
  void end(bool count);
  double averageMs() const { return avg_; }
  void release();

 private:
  void poll();
  static const int N = 6;
  GLuint q_[N] = {0};
  bool pending_[N] = {false};
  bool counted_[N] = {false};
  int head_ = 0, active_ = -1;
  int supported_ = -1;  // -1 unknown
  double avg_ = -1;
};

// Small, heavily blurred, slightly more saturated copy of the picture: the
// "ambient" backdrop the Liquid Glass UI is laid over and samples from.
struct Backdrop {
  GpuImage down[4], a, b;
  bool ready = false;
  void update(const GpuImage& in);
  const GpuImage& image() const { return b; }
  void release();
};

// Synchronous read of an image (snapshots).
void readImage(const GpuImage& img, std::vector<uint8_t>& out);
