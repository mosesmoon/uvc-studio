#include "Depth3D.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

// Depth cues on a coarse grid: each output texel summarises a 4×4 patch of
// the picture. R = depth guess (1 = near), G = text / HUD likeness,
// B = saturation.
const char* kCueFS = R"(#version 330 core
uniform sampler2D u_src;
uniform vec2 u_cell;   // one output texel, in uv
in vec2 v_uv;
out vec4 o;
float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }
void main() {
  float L[16];
  float sl = 0.0, sl2 = 0.0;
  vec3 sc = vec3(0.0);
  for (int j = 0; j < 4; ++j) {
    for (int i = 0; i < 4; ++i) {
      vec2 off = (vec2(float(i), float(j)) + 0.5) / 4.0 - 0.5;
      vec3 c = texture(u_src, v_uv + off * u_cell).rgb;
      float l = luma(c);
      L[j * 4 + i] = l;
      sl += l;
      sl2 += l * l;
      sc += c;
    }
  }
  float edge = 0.0;
  for (int j = 0; j < 4; ++j) {
    for (int i = 0; i < 4; ++i) {
      if (i < 3) edge += abs(L[j * 4 + i + 1] - L[j * 4 + i]);
      if (j < 3) edge += abs(L[(j + 1) * 4 + i] - L[j * 4 + i]);
    }
  }
  float mean = sl / 16.0;
  float sd = sqrt(max(sl2 / 16.0 - mean * mean, 0.0));
  vec3 mc = sc / 16.0;
  // Surround: the average colour a few cells away. Patches that stand out
  // from their surroundings are usually figures in front of a background.
  vec3 sur = vec3(0.0);
  for (int k = 0; k < 8; ++k) {
    float a = float(k) * 0.785398;
    sur += texture(u_src, v_uv + vec2(cos(a), sin(a)) * u_cell * 4.0).rgb;
  }
  sur /= 8.0;
  float salience = clamp(length(mc - sur) * 2.2, 0.0, 1.0);
  float hi = max(mc.r, max(mc.g, mc.b)), lo = min(mc.r, min(mc.g, mc.b));
  float sat = (hi - lo) / max(hi, 0.05);
  float detail = clamp(sd * 4.0 + edge / 24.0 * 3.0, 0.0, 1.0);
  float satN = clamp(sat * 1.2, 0.0, 1.0) * smoothstep(0.04, 0.20, mean);  // dark colours don't count
  float lower = smoothstep(0.0, 1.0, v_uv.y);                              // ground plane: lower = nearer
  vec2 cd = (v_uv - vec2(0.5, 0.6)) * vec2(1.1, 1.3);
  float centre = clamp(1.0 - length(cd), 0.0, 1.0);                        // subjects sit near the middle
  float haze = (1.0 - detail) * (1.0 - satN) * smoothstep(0.55, 0.90, mean);  // bright, flat, grey = sky / fog
  float d = 0.50 * lower + 0.22 * salience + 0.08 * detail + 0.10 * satN + 0.10 * centre - 0.25 * haze;
  // Text / HUD: thin, high-contrast strokes on a near-flat backing. The
  // patch is two-tone (almost every sample sits near the darkest or the
  // brightest value) and crosses between the two many times.
  float lo4 = L[0], hi4 = L[0];
  for (int i = 1; i < 16; ++i) {
    lo4 = min(lo4, L[i]);
    hi4 = max(hi4, L[i]);
  }
  float range = hi4 - lo4;
  float ends = 0.0;
  for (int i = 0; i < 16; ++i) ends += step(min(L[i] - lo4, hi4 - L[i]), 0.22 * range);
  float crossings = edge / max(range, 0.05);
  float hud = smoothstep(0.28, 0.50, range) * smoothstep(11.0, 14.5, ends) * smoothstep(4.5, 7.5, crossings);
  o = vec4(clamp(d, 0.0, 1.0), hud, satN, 1.0);
}
)";

// Separable gaussian (9 taps, 2-texel spacing) on the depth guess (R) and
// the text / HUD likeness (G).
const char* kBlurFS = R"(#version 330 core
uniform sampler2D u_src;
uniform vec2 u_dir;    // one texel along the blur axis, in uv
in vec2 v_uv;
out vec4 o;
void main() {
  float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
  vec2 d = texture(u_src, v_uv).rg * w[0];
  for (int i = 1; i < 5; ++i) {
    // Taps between texel pairs (1.5, 3.5, …): every texel is averaged in.
    // Whole-texel steps of 2 only mixed texels of the same parity and left a
    // one-texel checkerboard in the map — a ripple in the disparity every
    // few pixels, right through text.
    vec2 s = u_dir * (float(i) * 2.0 - 0.5);
    d += (texture(u_src, v_uv + s).rg + texture(u_src, v_uv - s).rg) * w[i];
  }
  o = vec4(d, 0.0, 1.0);
}
)";

// Range stretch, then pulls towards the screen plane (depth 0.5): text and
// HUD (the blurred likeness from the cue pass, so whole lines of text are
// covered; passed on in G for the gradient limit) and the left / right edges
// (no window violations). Temporal smoothing (no flicker between frames),
// and the flat region (webcam overlay) held exactly at screen depth.
const char* kTemporalFS = R"(#version 330 core
uniform sampler2D u_cur;
uniform sampler2D u_prev;
uniform float u_keep;      // 0 = no history
uniform vec4 u_flat;       // uv rect held at screen depth
uniform float u_hasFlat;
in vec2 v_uv;
out vec4 o;
void main() {
  vec2 c = texture(u_cur, v_uv).rg;
  float d = clamp((c.r - 0.10) / 0.62, 0.0, 1.0);
  float hud = smoothstep(0.04, 0.16, c.g);
  d = mix(d, 0.5, 0.9 * hud);
  float edge = smoothstep(0.0, 0.06, v_uv.x) * smoothstep(0.0, 0.06, 1.0 - v_uv.x);
  d = mix(0.5, d, edge);
  d = mix(d, texture(u_prev, v_uv).r, u_keep);
  if (u_hasFlat > 0.5 && v_uv.x >= u_flat.x && v_uv.x <= u_flat.z && v_uv.y >= u_flat.y && v_uv.y <= u_flat.w)
    d = 0.5;
  o = vec4(d, hud, 0.0, 1.0);
}
)";

// Disparity gradient limit along one axis. The result is the average of the
// upper and lower "cone" envelopes of the map (max / min over neighbours of
// depth -/+ slope x distance), so it changes by at most u_slope per texel:
// the stereo warp then never stretches or squeezes the picture by more than
// a few percent, and a character or HUD element moves as one rigid piece
// instead of tearing where the depth changes. Both envelopes stay close to
// the input wherever it is already smooth.
const char* kLimitFS = R"(#version 330 core
uniform sampler2D u_src;
uniform ivec2 u_axis;      // (1, 0) horizontal, (0, 1) vertical
uniform float u_slope;     // largest change per texel
uniform int u_radius;      // texels searched each way (>= 1 / u_slope)
out vec4 o;
void main() {
  ivec2 p = ivec2(gl_FragCoord.xy);
  ivec2 last = textureSize(u_src, 0) - 1;
  vec2 dh = texelFetch(u_src, p, 0).rg;  // depth, text / HUD likeness
  // Text and HUD get a much flatter slope: they move as one layer.
  float slope = u_slope * mix(1.0, 0.25, dh.g);
  float hi = dh.r, lo = dh.r;
  for (int i = 1; i <= u_radius; ++i) {
    float a = texelFetch(u_src, clamp(p + u_axis * i, ivec2(0), last), 0).r;
    float b = texelFetch(u_src, clamp(p - u_axis * i, ivec2(0), last), 0).r;
    float m = float(i) * slope;
    hi = max(hi, max(a, b) - m);
    lo = min(lo, min(a, b) + m);
  }
  o = vec4(0.5 * (hi + lo), dh.g, 0.0, 1.0);
}
)";

// 2D depth enhancer.
const char* kEnhanceFS = R"(#version 330 core
uniform sampler2D u_src;
uniform sampler2D u_depth;
uniform vec2 u_texel;
uniform float u_strength;
in vec2 v_uv;
out vec4 o;
const vec3 W = vec3(0.2126, 0.7152, 0.0722);
void main() {
  vec3 c = texture(u_src, v_uv).rgb;
  float d = texture(u_depth, v_uv).r;
  float near = smoothstep(0.30, 0.85, d);
  float far = 1.0 - smoothstep(0.15, 0.60, d);
  vec2 t = u_texel * 1.5;
  vec3 b1 = (texture(u_src, v_uv + vec2(t.x, 0.0)).rgb + texture(u_src, v_uv - vec2(t.x, 0.0)).rgb +
             texture(u_src, v_uv + vec2(0.0, t.y)).rgb + texture(u_src, v_uv - vec2(0.0, t.y)).rgb) * 0.25;
  vec2 t2 = u_texel * 7.0;   // wide taps: background defocus
  vec3 b2 = (texture(u_src, v_uv + t2).rgb + texture(u_src, v_uv - t2).rgb +
             texture(u_src, v_uv + vec2(t2.x, -t2.y)).rgb + texture(u_src, v_uv + vec2(-t2.x, t2.y)).rgb +
             texture(u_src, v_uv + vec2(t2.x * 1.4, 0.0)).rgb + texture(u_src, v_uv - vec2(t2.x * 1.4, 0.0)).rgb +
             texture(u_src, v_uv + vec2(0.0, t2.y * 1.4)).rgb + texture(u_src, v_uv - vec2(0.0, t2.y * 1.4)).rgb +
             texture(u_src, v_uv + t2 * 0.5).rgb + texture(u_src, v_uv - t2 * 0.5).rgb +
             texture(u_src, v_uv + vec2(t2.x, -t2.y) * 0.5).rgb + texture(u_src, v_uv + vec2(-t2.x, t2.y) * 0.5).rgb) / 12.0;
  vec3 blur = mix(b1, b2, 0.7);
  float s = u_strength;
  // Foreground: crisper edges, more local contrast and colour.
  vec3 col = c + (c - b1) * (1.1 * s * near);
  float lum = dot(col, W);
  col = mix(vec3(lum), col, 1.0 + 0.22 * s * near);
  // Background: gentle defocus, less colour, a hint of atmospheric haze.
  col = mix(col, blur, min(1.0, 1.3 * s * far));
  lum = dot(col, W);
  col = mix(col, vec3(lum), 0.60 * s * far);
  col = mix(col, vec3(lum) * vec3(0.94, 0.98, 1.06) + 0.04, 0.44 * s * far);
  // Nearer a touch brighter, farther a touch dimmer.
  col *= 1.0 + 0.08 * s * (d - 0.5);
  o = vec4(clamp(col, 0.0, 1.0), 1.0);
}
)";

// One eye of a stereo pair: each output pixel samples the picture shifted by
// its disparity (depth relative to the screen plane). The fixed-point loop
// inverts the forward shift so near objects move as a whole; it converges
// because the depth map's gradient is limited (kLimitFS). The map is read
// with a cubic B-spline (four bilinear taps), so the shift has no kinks at
// the coarse map's texel boundaries — with plain bilinear reads every
// boundary bent the strokes of text passing through it.
const char* kStereoFS = R"(#version 330 core
uniform sampler2D u_src;
uniform sampler2D u_depth;
uniform vec2 u_depthSize;  // depth map size in texels
uniform float u_eye;       // -1 left, +1 right
uniform float u_amount;    // largest per-eye shift (fraction of width)
in vec2 v_uv;
out vec4 o;
float depthAt(vec2 uv) {
  vec2 t = uv * u_depthSize - 0.5;
  vec2 f = fract(t);
  vec2 f2 = f * f, f3 = f2 * f;
  vec2 w0 = (1.0 - 3.0 * f + 3.0 * f2 - f3) / 6.0;
  vec2 w1 = (4.0 - 6.0 * f2 + 3.0 * f3) / 6.0;
  vec2 w2 = (1.0 + 3.0 * f + 3.0 * f2 - 3.0 * f3) / 6.0;
  vec2 w3 = f3 / 6.0;
  vec2 g0 = w0 + w1, g1 = w2 + w3;
  vec2 c = floor(t) + 0.5;
  vec2 p0 = (c - 1.0 + w1 / g0) / u_depthSize;
  vec2 p1 = (c + 1.0 + w3 / g1) / u_depthSize;
  return g0.y * (g0.x * texture(u_depth, p0).r + g1.x * texture(u_depth, vec2(p1.x, p0.y)).r) +
         g1.y * (g0.x * texture(u_depth, vec2(p0.x, p1.y)).r + g1.x * texture(u_depth, p1).r);
}
void main() {
  float k = u_eye * u_amount;
  float x = v_uv.x;
  for (int i = 0; i < 4; ++i) x = v_uv.x + k * (depthAt(vec2(x, v_uv.y)) - 0.5);
  o = vec4(texture(u_src, vec2(clamp(x, 0.0, 1.0), v_uv.y)).rgb, 1.0);
}
)";

const char* kAnaglyphFS = R"(#version 330 core
uniform sampler2D u_left;
uniform sampler2D u_right;
in vec2 v_uv;
out vec4 o;
void main() {
  vec3 l = texture(u_left, v_uv).rgb;
  vec3 r = texture(u_right, v_uv).rgb;
  o = vec4(dot(l, vec3(0.299, 0.587, 0.114)), r.g, r.b, 1.0);
}
)";

// Depth maps are kept in half floats where the GPU can render to them: the
// gradient limit works in steps of ~1/50 of the depth range, which 8-bit
// texels would round coarsely enough to show as small bends in the stereo
// warp. (OpenGL ES 3.0 needs an extension for half-float render targets.)
GLenum mapFormat() {
  static GLenum fmt = 0;
  if (fmt) return fmt;
  fmt = GL_RGBA16F;
  return fmt;
}

void bindTex(int unit, GLuint tex) {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, tex);
}

}  // namespace

DepthPrograms& DepthPrograms::get() {
  static DepthPrograms p;
  return p;
}

bool DepthPrograms::init() {
  cue = linkProgram(kCueFS);
  blur = linkProgram(kBlurFS);
  temporal = linkProgram(kTemporalFS);
  enhance = linkProgram(kEnhanceFS);
  limit = linkProgram(kLimitFS);
  stereo = linkProgram(kStereoFS);
  anaglyph = linkProgram(kAnaglyphFS);
  return cue && blur && temporal && limit && enhance && stereo && anaglyph;
}

void DepthPrograms::shutdown() {
  for (GLuint* p : {&cue, &blur, &temporal, &limit, &enhance, &stereo, &anaglyph})
    if (*p) glDeleteProgram(*p), *p = 0;
}

DepthEngine::~DepthEngine() { release(); }

void DepthEngine::release() {
  cue_.release();
  tmp_.release();
  hist_[0].release();
  hist_[1].release();
  final_.release();
  have_ = false;
}

void DepthEngine::update(const GpuImage& src, const float* flat) {
  auto& D = DepthPrograms::get();
  auto& P = GpuPrograms::get();
  if (!src.tex || src.w <= 0 || src.h <= 0 || !D.cue) return;
  GlStateGuard guard;
  const int cw = 192;
  const int ch = std::max(8, (int)std::lround(cw * (double)src.h / src.w));
  const GLenum fmt = mapFormat();
  cue_.ensure(cw, ch, fmt);
  tmp_.ensure(cw, ch, fmt);
  if (hist_[cur_].w != cw || hist_[cur_].h != ch || hist_[cur_].fmt != fmt) have_ = false;
  hist_[0].ensure(cw, ch, fmt);
  hist_[1].ensure(cw, ch, fmt);
  glViewport(0, 0, cw, ch);

  // Cues.
  glBindFramebuffer(GL_FRAMEBUFFER, cue_.fbo);
  glUseProgram(D.cue);
  bindTex(0, src.tex);
  glUniform1i(glGetUniformLocation(D.cue, "u_src"), 0);
  glUniform2f(glGetUniformLocation(D.cue, "u_cell"), 1.f / cw, 1.f / ch);
  P.quad(D.cue, cw, ch, 0, 0, (float)cw, (float)ch);

  // Blur: horizontal cue → tmp, vertical tmp → cue.
  glUseProgram(D.blur);
  glUniform1i(glGetUniformLocation(D.blur, "u_src"), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, tmp_.fbo);
  bindTex(0, cue_.tex);
  glUniform2f(glGetUniformLocation(D.blur, "u_dir"), 1.f / cw, 0.f);
  P.quad(D.blur, cw, ch, 0, 0, (float)cw, (float)ch);
  glBindFramebuffer(GL_FRAMEBUFFER, cue_.fbo);
  bindTex(0, tmp_.tex);
  glUniform2f(glGetUniformLocation(D.blur, "u_dir"), 0.f, 1.f / ch);
  P.quad(D.blur, cw, ch, 0, 0, (float)cw, (float)ch);

  // Temporal blend into the next history slot.
  int next = 1 - cur_;
  glBindFramebuffer(GL_FRAMEBUFFER, hist_[next].fbo);
  glUseProgram(D.temporal);
  bindTex(0, cue_.tex);
  bindTex(1, hist_[cur_].tex);
  glUniform1i(glGetUniformLocation(D.temporal, "u_cur"), 0);
  glUniform1i(glGetUniformLocation(D.temporal, "u_prev"), 1);
  glUniform1f(glGetUniformLocation(D.temporal, "u_keep"), have_ ? 0.8f : 0.f);
  if (flat) {
    glUniform4f(glGetUniformLocation(D.temporal, "u_flat"), flat[0], flat[1], flat[2], flat[3]);
    glUniform1f(glGetUniformLocation(D.temporal, "u_hasFlat"), 1.f);
  } else {
    glUniform1f(glGetUniformLocation(D.temporal, "u_hasFlat"), 0.f);
  }
  P.quad(D.temporal, cw, ch, 0, 0, (float)cw, (float)ch);
  bindTex(1, 0);
  cur_ = next;

  // Gradient limit: horizontal hist → tmp, vertical tmp → final. The slope
  // keeps the stereo warp within ±kMaxStretch at the largest shift any view
  // uses (2 % of the width per eye: Quest immersive mode at full strength).
  const float kMaxStretch = 0.07f, kMaxAmount = 0.02f;
  float slope = kMaxStretch / (kMaxAmount * cw);
  int radius = std::min(128, (int)std::ceil(1.f / slope));
  final_.ensure(cw, ch, fmt);
  glUseProgram(D.limit);
  glUniform1i(glGetUniformLocation(D.limit, "u_src"), 0);
  glUniform1i(glGetUniformLocation(D.limit, "u_radius"), radius);
  glBindFramebuffer(GL_FRAMEBUFFER, tmp_.fbo);
  bindTex(0, hist_[cur_].tex);
  glUniform2i(glGetUniformLocation(D.limit, "u_axis"), 1, 0);
  glUniform1f(glGetUniformLocation(D.limit, "u_slope"), slope);
  P.quad(D.limit, cw, ch, 0, 0, (float)cw, (float)ch);
  glBindFramebuffer(GL_FRAMEBUFFER, final_.fbo);
  bindTex(0, tmp_.tex);
  glUniform2i(glGetUniformLocation(D.limit, "u_axis"), 0, 1);
  glUniform1f(glGetUniformLocation(D.limit, "u_slope"), slope);
  P.quad(D.limit, cw, ch, 0, 0, (float)cw, (float)ch);
  // Round off the envelopes' corners (a blur keeps the gradient limit):
  // final → tmp → final.
  glUseProgram(D.blur);
  glUniform1i(glGetUniformLocation(D.blur, "u_src"), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, tmp_.fbo);
  bindTex(0, final_.tex);
  glUniform2f(glGetUniformLocation(D.blur, "u_dir"), 1.f / cw, 0.f);
  P.quad(D.blur, cw, ch, 0, 0, (float)cw, (float)ch);
  glBindFramebuffer(GL_FRAMEBUFFER, final_.fbo);
  bindTex(0, tmp_.tex);
  glUniform2f(glGetUniformLocation(D.blur, "u_dir"), 0.f, 1.f / ch);
  P.quad(D.blur, cw, ch, 0, 0, (float)cw, (float)ch);
  have_ = true;
}

void DepthEngine::enhance(const GpuImage& src, GpuImage& out, float strength) {
  auto& D = DepthPrograms::get();
  if (!have_ || !D.enhance) return;
  GlStateGuard guard;
  out.ensure(src.w, src.h);
  glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);
  glViewport(0, 0, src.w, src.h);
  glUseProgram(D.enhance);
  bindTex(0, src.tex);
  bindTex(1, final_.tex);
  glUniform1i(glGetUniformLocation(D.enhance, "u_src"), 0);
  glUniform1i(glGetUniformLocation(D.enhance, "u_depth"), 1);
  glUniform2f(glGetUniformLocation(D.enhance, "u_texel"), 1.f / src.w, 1.f / src.h);
  glUniform1f(glGetUniformLocation(D.enhance, "u_strength"), std::clamp(strength, 0.f, 1.f));
  GpuPrograms::get().quad(D.enhance, src.w, src.h, 0, 0, (float)src.w, (float)src.h);
  bindTex(1, 0);
}

void DepthEngine::drawStereo(const GpuImage& src, int eye, float amount, int w, int h, bool flipV, int x, int y) {
  auto& D = DepthPrograms::get();
  if (!D.stereo) return;
  GlStateGuard guard;
  glViewport(x, y, w, h);
  glUseProgram(D.stereo);
  bindTex(0, src.tex);
  bindTex(1, have_ ? final_.tex : 0);
  glUniform1i(glGetUniformLocation(D.stereo, "u_src"), 0);
  glUniform1i(glGetUniformLocation(D.stereo, "u_depth"), 1);
  glUniform2f(glGetUniformLocation(D.stereo, "u_depthSize"), (float)std::max(1, final_.w),
              (float)std::max(1, final_.h));
  glUniform1f(glGetUniformLocation(D.stereo, "u_eye"), eye < 0 ? -1.f : 1.f);
  glUniform1f(glGetUniformLocation(D.stereo, "u_amount"), have_ ? amount : 0.f);
  if (flipV)
    GpuPrograms::get().quad(D.stereo, w, h, 0, 0, (float)w, (float)h, 0, 1, 1, 0);
  else
    GpuPrograms::get().quad(D.stereo, w, h, 0, 0, (float)w, (float)h);
  bindTex(1, 0);
}

void DepthEngine::anaglyph(const GpuImage& left, const GpuImage& right, GpuImage& out) {
  auto& D = DepthPrograms::get();
  if (!D.anaglyph || !left.tex || !right.tex) return;
  GlStateGuard guard;
  out.ensure(left.w, left.h);
  glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);
  glViewport(0, 0, out.w, out.h);
  glUseProgram(D.anaglyph);
  bindTex(0, left.tex);
  bindTex(1, right.tex);
  glUniform1i(glGetUniformLocation(D.anaglyph, "u_left"), 0);
  glUniform1i(glGetUniformLocation(D.anaglyph, "u_right"), 1);
  GpuPrograms::get().quad(D.anaglyph, out.w, out.h, 0, 0, (float)out.w, (float)out.h);
  bindTex(1, 0);
}
