// "3D depth": a monocular depth estimate of the live picture, used two ways:
//   - 2D displays: a depth enhancer (crisper, more contrasty foreground,
//     softer and hazier background) that makes a flat picture read as deep;
//   - Quest immersive mode: a real stereo pair (left / right eye views with
//     per-pixel parallax) so the giant screen has actual depth.
// The estimate combines cues that hold for most game and video content
// (lower in frame = nearer, detailed / saturated = nearer, bright flat haze =
// farther), smoothed spatially and over time. Text and HUD elements are held
// on the screen plane, and the map's gradient is limited so the stereo warp
// moves small detail rigidly (no seams through characters). It runs at low
// resolution, so the cost is a few small passes per frame plus one
// full-size pass.
#pragma once
#include "VideoRenderer.h"

class DepthPrograms {
 public:
  static DepthPrograms& get();
  bool init();
  void shutdown();
  GLuint cue = 0, blur = 0, temporal = 0, limit = 0, enhance = 0, stereo = 0, anaglyph = 0;
};

class DepthEngine {
 public:
  ~DepthEngine();
  // Updates the depth map from `src` (call once per new frame). `flat`
  // (uv rect x0, y0, x1, y1) is held at screen depth — used for the PiP
  // overlay so it stays a flat window. null = none.
  void update(const GpuImage& src, const float* flat = nullptr);
  bool ready() const { return have_; }
  const GpuImage& depth() const { return final_; }
  // Depth-enhanced copy of `src` into `out` (same size). strength 0 … 1.
  void enhance(const GpuImage& src, GpuImage& out, float strength);
  // Draws one eye of a stereo view of `src` into the currently bound
  // framebuffer (w × h). eye: -1 left, +1 right. `amount` = largest
  // per-eye shift as a fraction of the width. flipV for GL-oriented targets
  // (OpenXR swapchains, window surfaces).
  // (x, y) offsets the viewport, for side-by-side / top-and-bottom layouts.
  void drawStereo(const GpuImage& src, int eye, float amount, int w, int h, bool flipV, int x = 0, int y = 0);
  // Red / cyan anaglyph of a left / right pair (half-colour: left eye's
  // luminance in red, right eye's green and blue).
  static void anaglyph(const GpuImage& left, const GpuImage& right, GpuImage& out);
  void reset() { have_ = false; }
  void release();

 private:
  GpuImage cue_, tmp_, hist_[2];
  GpuImage final_;  // gradient-limited map used by the effect and the stereo pair
  int cur_ = 0;
  bool have_ = false;
};
