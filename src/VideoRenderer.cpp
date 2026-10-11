#include "VideoRenderer.h"

#include "Depth3D.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

const char* kVS = R"(#version 330 core
uniform vec4 u_dst;   // NDC x0,y0,x1,y1
uniform vec4 u_uvr;   // u0,v0,u1,v1
out vec2 v_uv;
void main() {
  vec2 c = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
  v_uv = mix(u_uvr.xy, u_uvr.zw, c);
  gl_Position = vec4(mix(u_dst.xy, u_dst.zw, c), 0.0, 1.0);
}
)";

// Captured frame → RGB. Images are stored top-down (row 0 = top) everywhere.
const char* kConvertFS = R"(#version 330 core
uniform int u_fmt;      // 0 YUYV, 1 UYVY, 2 NV12/21, 3 planar YUV, 4 RGB, 5 gray
uniform int u_swap;
uniform ivec2 u_csub;   // chroma subsampling shift (planar)
uniform vec2 u_size;
uniform sampler2D t0;
uniform sampler2D t1;
uniform sampler2D t2;
uniform mat3 u_m;
uniform vec3 u_off;
in vec2 v_uv;
out vec4 o;
void main() {
  ivec2 p = ivec2(gl_FragCoord.xy);
  vec2 nuv = gl_FragCoord.xy / u_size;
  vec3 yuv;
  if (u_fmt == 0) {
    vec4 t = texelFetch(t0, ivec2(p.x >> 1, p.y), 0);
    yuv = vec3(((p.x & 1) == 0) ? t.r : t.b, t.g, t.a);
  } else if (u_fmt == 1) {
    vec4 t = texelFetch(t0, ivec2(p.x >> 1, p.y), 0);
    yuv = vec3(((p.x & 1) == 0) ? t.g : t.a, t.r, t.b);
  } else if (u_fmt == 2) {
    float y = texelFetch(t0, p, 0).r;
    vec2 c = texture(t1, nuv).rg;
    if (u_swap == 1) c = c.yx;
    yuv = vec3(y, c);
  } else if (u_fmt == 3) {
    yuv = vec3(texelFetch(t0, p, 0).r, texture(t1, nuv).r, texture(t2, nuv).r);
  } else if (u_fmt == 4) {
    vec3 c = texelFetch(t0, p, 0).rgb;
    if (u_swap == 1) c = c.bgr;
    o = vec4(c, 1.0);
    return;
  } else {
    float g = texelFetch(t0, p, 0).r;
    o = vec4(g, g, g, 1.0);
    return;
  }
  vec3 rgb = u_m * (yuv - u_off);
  o = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
)";

// Picture controls: brightness, contrast and saturation as gains around
// 1.0 (0.60 … 1.40). Saturation is applied first (around the Rec. 709 luma),
// then contrast around mid-grey, then the overall gain, and finally gamma
// (c^(1 / gamma), 0.60 … 1.40; above 1 lifts the mid-tones). Shared by every
// pass that writes the final colour.
#define UVC_PICTURE_GLSL                                       \
  "uniform float u_gain;\n"                                    \
  "uniform float u_contrast_gain;\n"                           \
  "uniform float u_sat_gain;\n"                                \
  "uniform float u_gamma;\n"                                   \
  "vec3 pictureAdjust(vec3 c) {\n"                             \
  "  float y = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"        \
  "  c = max(vec3(y) + (c - vec3(y)) * u_sat_gain, 0.0);\n"    \
  "  c = (c - vec3(0.5)) * u_contrast_gain + vec3(0.5);\n"     \
  "  c = clamp(c * u_gain, 0.0, 1.0);\n"                       \
  "  return pow(c, vec3(1.0 / u_gamma));\n"                    \
  "}\n"

// Plain picture-controls pass (resamples bilinearly to the target size).
const char* kBcsFS = R"(#version 330 core
uniform sampler2D u_video;
in vec2 v_uv;
out vec4 o;
)" UVC_PICTURE_GLSL R"(
void main() { o = vec4(pictureAdjust(texture(u_video, v_uv).rgb), 1.0); }
)";

// "FSR 4K upscaling": AMD FidelityFX Super Resolution 1.0 (MIT licence,
// Copyright (c) 2021 Advanced Micro Devices, Inc.), ported from ffx_fsr1.h
// (the 32-bit float paths) to plain GLSL so it runs on OpenGL 3.3 and
// OpenGL ES 3.0 alike. Images are top-down, which matches FSR's pixel
// convention, so integer pixel positions map directly.
//
// EASU: edge-adaptive spatial upsampling. A 12-tap Lanczos-like kernel whose
// footprint is rotated and stretched along the local edge direction, then
// clamped to the 2×2 neighbourhood to avoid ringing.
const char* kEasuFS = R"(#version 330 core
uniform sampler2D u_video;
uniform vec4 u_con0;    // input/output scale (xy), offset (zw)
uniform ivec2 u_max;    // last input texel
in vec2 v_uv;
out vec4 o;
vec3 tap(ivec2 p) { return texelFetch(u_video, clamp(p, ivec2(0), u_max), 0).rgb; }
// Luma times two, as in FSR.
float lum2(vec3 c) { return c.b * 0.5 + (c.r * 0.5 + c.g); }
float rcpLo(float a) { return uintBitsToFloat(0x7ef07ebbu - floatBitsToUint(a)); }
float rsqLo(float a) { return uintBitsToFloat(0x5f347d74u - (floatBitsToUint(a) >> 1u)); }
void easuSet(inout vec2 dir, inout float len, float w, float lA, float lB, float lC, float lD, float lE) {
  float dc = lD - lC;
  float cb = lC - lB;
  float lenX = rcpLo(max(abs(dc), abs(cb)));
  float dirX = lD - lB;
  dir.x += dirX * w;
  lenX = clamp(abs(dirX) * lenX, 0.0, 1.0);
  lenX *= lenX;
  len += lenX * w;
  float ec = lE - lC;
  float ca = lC - lA;
  float lenY = rcpLo(max(abs(ec), abs(ca)));
  float dirY = lE - lA;
  dir.y += dirY * w;
  lenY = clamp(abs(dirY) * lenY, 0.0, 1.0);
  lenY *= lenY;
  len += lenY * w;
}
void easuTap(inout vec3 aC, inout float aW, vec2 off, vec2 dir, vec2 len, float lob, float clp, vec3 c) {
  vec2 v = vec2(off.x * dir.x + off.y * dir.y, off.x * (-dir.y) + off.y * dir.x);
  v *= len;
  float d2 = min(v.x * v.x + v.y * v.y, clp);
  float wB = 0.4 * d2 - 1.0;
  float wA = lob * d2 - 1.0;
  wB *= wB;
  wA *= wA;
  wB = 1.5625 * wB - 0.5625;
  float w = wB * wA;
  aC += c * w;
  aW += w;
}
void main() {
  vec2 pp = floor(gl_FragCoord.xy) * u_con0.xy + u_con0.zw;
  vec2 fp = floor(pp);
  pp -= fp;
  ivec2 F = ivec2(fp);
  //    b c
  //  e f g h
  //  i j k l
  //    n o
  vec3 b = tap(F + ivec2(0, -1)), c = tap(F + ivec2(1, -1));
  vec3 e = tap(F + ivec2(-1, 0)), f = tap(F), g = tap(F + ivec2(1, 0)), h = tap(F + ivec2(2, 0));
  vec3 i = tap(F + ivec2(-1, 1)), j = tap(F + ivec2(0, 1)), k = tap(F + ivec2(1, 1)), l = tap(F + ivec2(2, 1));
  vec3 n = tap(F + ivec2(0, 2)), oo = tap(F + ivec2(1, 2));
  float bL = lum2(b), cL = lum2(c), eL = lum2(e), fL = lum2(f), gL = lum2(g), hL = lum2(h);
  float iL = lum2(i), jL = lum2(j), kL = lum2(k), lL = lum2(l), nL = lum2(n), oL = lum2(oo);
  vec2 dir = vec2(0.0);
  float len = 0.0;
  easuSet(dir, len, (1.0 - pp.x) * (1.0 - pp.y), bL, eL, fL, gL, jL);
  easuSet(dir, len, pp.x * (1.0 - pp.y), cL, fL, gL, hL, kL);
  easuSet(dir, len, (1.0 - pp.x) * pp.y, fL, iL, jL, kL, nL);
  easuSet(dir, len, pp.x * pp.y, gL, jL, kL, lL, oL);
  vec2 dir2 = dir * dir;
  float dirR = dir2.x + dir2.y;
  bool zro = dirR < (1.0 / 32768.0);
  dirR = zro ? 1.0 : rsqLo(dirR);
  dir.x = zro ? 1.0 : dir.x;
  dir *= vec2(dirR);
  len = len * 0.5;
  len *= len;
  float stretch = (dir.x * dir.x + dir.y * dir.y) * rcpLo(max(abs(dir.x), abs(dir.y)));
  vec2 len2 = vec2(1.0 + (stretch - 1.0) * len, 1.0 - 0.5 * len);
  float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
  float clp = rcpLo(lob);
  vec3 min4 = min(min(f, g), min(j, k));
  vec3 max4 = max(max(f, g), max(j, k));
  vec3 aC = vec3(0.0);
  float aW = 0.0;
  easuTap(aC, aW, vec2(0.0, -1.0) - pp, dir, len2, lob, clp, b);
  easuTap(aC, aW, vec2(1.0, -1.0) - pp, dir, len2, lob, clp, c);
  easuTap(aC, aW, vec2(-1.0, 1.0) - pp, dir, len2, lob, clp, i);
  easuTap(aC, aW, vec2(0.0, 1.0) - pp, dir, len2, lob, clp, j);
  easuTap(aC, aW, vec2(0.0, 0.0) - pp, dir, len2, lob, clp, f);
  easuTap(aC, aW, vec2(-1.0, 0.0) - pp, dir, len2, lob, clp, e);
  easuTap(aC, aW, vec2(1.0, 1.0) - pp, dir, len2, lob, clp, k);
  easuTap(aC, aW, vec2(2.0, 1.0) - pp, dir, len2, lob, clp, l);
  easuTap(aC, aW, vec2(2.0, 0.0) - pp, dir, len2, lob, clp, h);
  easuTap(aC, aW, vec2(1.0, 0.0) - pp, dir, len2, lob, clp, g);
  easuTap(aC, aW, vec2(1.0, 2.0) - pp, dir, len2, lob, clp, oo);
  easuTap(aC, aW, vec2(0.0, 2.0) - pp, dir, len2, lob, clp, n);
  o = vec4(min(max4, max(min4, aC / aW)), 1.0);
}
)";

// RCAS: robust contrast-adaptive sharpening (FSR's second pass), limited so
// it never pushes a pixel past its neighbourhood's range. Followed by the
// colour adjustments.
const char* kRcasFS = R"(#version 330 core
uniform sampler2D u_video;
uniform float u_sharp;      // FsrRcasCon: exp2(-stops)
uniform ivec2 u_max;
in vec2 v_uv;
out vec4 o;
)" UVC_PICTURE_GLSL R"(
vec3 ld(ivec2 p) { return texelFetch(u_video, clamp(p, ivec2(0), u_max), 0).rgb; }
float rcpMed(float a) {
  float b = uintBitsToFloat(0x7ef19fffu - floatBitsToUint(a));
  return b * (-b * a + 2.0);
}
void main() {
  ivec2 sp = ivec2(gl_FragCoord.xy);
  vec3 b = ld(sp + ivec2(0, -1));
  vec3 d = ld(sp + ivec2(-1, 0));
  vec3 e = ld(sp);
  vec3 f = ld(sp + ivec2(1, 0));
  vec3 h = ld(sp + ivec2(0, 1));
  vec3 mn4 = min(min(b, d), min(f, h));
  vec3 mx4 = max(max(b, d), max(f, h));
  vec2 peakC = vec2(1.0, -4.0);
  // (Guarded against 0/0 on pure black / pure white neighbourhoods.)
  vec3 hitMin = min(mn4, e) / max(4.0 * mx4, vec3(1e-5));
  vec3 hitMax = (peakC.x - max(mx4, e)) / min(4.0 * mn4 + peakC.y, vec3(-1e-5));
  vec3 lobeRGB = max(-hitMin, hitMax);
  float lobe = max(-(0.25 - (1.0 / 16.0)), min(max(lobeRGB.r, max(lobeRGB.g, lobeRGB.b)), 0.0)) * u_sharp;
  float rcpL = rcpMed(4.0 * lobe + 1.0);
  vec3 color = (lobe * (b + d + h + f) + e) * rcpL;
  o = vec4(pictureAdjust(clamp(color, 0.0, 1.0)), 1.0);
}
)";

// "Lanczos + CAS": separable Lanczos-3 resampling (6 taps per axis, one pass
// per axis), then AMD FidelityFX CAS (MIT licence, Copyright (c) 2020
// Advanced Micro Devices, Inc.) sharpening at the output resolution.
const char* kLanczosFS = R"(#version 330 core
uniform sampler2D u_video;
uniform ivec2 u_max;       // last input texel
uniform float u_scale;     // input / output along the axis
uniform int u_axis;        // 0 horizontal, 1 vertical
in vec2 v_uv;
out vec4 o;
float lanczos3(float x) {
  x = abs(x);
  if (x < 1e-5) return 1.0;
  if (x >= 3.0) return 0.0;
  float px = 3.14159265 * x;
  return 3.0 * sin(px) * sin(px / 3.0) / (px * px);
}
void main() {
  ivec2 op = ivec2(gl_FragCoord.xy);
  float c = (float(u_axis == 0 ? op.x : op.y) + 0.5) * u_scale - 0.5;
  float fc = floor(c);
  float f = c - fc;
  int i0 = int(fc);
  vec3 acc = vec3(0.0);
  float wsum = 0.0;
  for (int k = -2; k <= 3; ++k) {
    float w = lanczos3(f - float(k));
    ivec2 sp = u_axis == 0 ? ivec2(i0 + k, op.y) : ivec2(op.x, i0 + k);
    acc += w * texelFetch(u_video, clamp(sp, ivec2(0), u_max), 0).rgb;
    wsum += w;
  }
  o = vec4(clamp(acc / wsum, 0.0, 1.0), 1.0);
}
)";

// CAS, sharpen-only path with CAS_BETTER_DIAGONALS and exact maths, followed
// by the colour adjustments.
const char* kCasFS = R"(#version 330 core
uniform sampler2D u_video;
uniform ivec2 u_max;
uniform float u_peak;       // CasSetup: -1 / mix(8, 5, sharpness)
in vec2 v_uv;
out vec4 o;
)" UVC_PICTURE_GLSL R"(
vec3 ld(ivec2 p) { return texelFetch(u_video, clamp(p, ivec2(0), u_max), 0).rgb; }
void main() {
  ivec2 sp = ivec2(gl_FragCoord.xy);
  vec3 a = ld(sp + ivec2(-1, -1)), b = ld(sp + ivec2(0, -1)), c = ld(sp + ivec2(1, -1));
  vec3 d = ld(sp + ivec2(-1, 0)), e = ld(sp), f = ld(sp + ivec2(1, 0));
  vec3 g = ld(sp + ivec2(-1, 1)), h = ld(sp + ivec2(0, 1)), i = ld(sp + ivec2(1, 1));
  vec3 mn = min(min(min(d, e), min(f, b)), h);
  vec3 mn2 = min(mn, min(min(a, c), min(g, i)));
  mn += mn2;
  vec3 mx = max(max(max(d, e), max(f, b)), h);
  vec3 mx2 = max(mx, max(max(a, c), max(g, i)));
  mx += mx2;
  vec3 amp = sqrt(clamp(min(mn, 2.0 - mx) / max(mx, vec3(1e-5)), 0.0, 1.0));
  float w = amp.g * u_peak;
  vec3 color = clamp(((b + d + f + h) * w + e) / (1.0 + 4.0 * w), 0.0, 1.0);
  o = vec4(pictureAdjust(color), 1.0);
}
)";

// "NIS": NVIDIA Image Scaling v1.0.3 (MIT licence, Copyright (c) 2022 NVIDIA
// CORPORATION & AFFILIATES), NVScaler ported from its compute-shader form
// (shared-memory tiles) to one fragment shader per output pixel: the 6 × 6
// source luma neighbourhood, four-direction edge detection, 6-tap
// polyphase scaler + directional filters with luma-adaptive unsharp mask
// and anti-ringing, applied as a luma correction on the bilinear colour.
// Scales from 1× to 2×.
const char* kNisFS = R"(#version 330 core
uniform sampler2D u_video;
uniform sampler2D u_coef;   // 4 × 64 RGBA32F filter banks (NISCoef.inc)
uniform ivec2 u_max;
uniform vec2 u_scale;       // input / output
uniform float kDetectRatio;
uniform float kDetectThres;
uniform float kMinContrastRatio;
uniform float kRatioNorm;
uniform float kSharpStartY;
uniform float kSharpScaleY;
uniform float kSharpStrengthMin;
uniform float kSharpStrengthScale;
uniform float kSharpLimitMin;
uniform float kSharpLimitScale;
in vec2 v_uv;
out vec4 o;
const float kEps = 1.0 / 255.0;
const float kContrastBoost = 1.0;
const int kPhaseCount = 64;
float P[36];
float px(int r, int c) { return P[r * 6 + c]; }
float getY(vec3 c) { return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b; }

vec4 edgeMap(int i, int j) {
  float g_0 = abs(px(i, j) + px(i, j + 1) + px(i, j + 2) - px(i + 2, j) - px(i + 2, j + 1) - px(i + 2, j + 2));
  float g_45 = abs(px(i + 1, j) + px(i, j) + px(i, j + 1) - px(i + 2, j + 1) - px(i + 2, j + 2) - px(i + 1, j + 2));
  float g_90 = abs(px(i, j) + px(i + 1, j) + px(i + 2, j) - px(i, j + 2) - px(i + 1, j + 2) - px(i + 2, j + 2));
  float g_135 = abs(px(i + 1, j) + px(i + 2, j) + px(i + 2, j + 1) - px(i, j + 1) - px(i, j + 2) - px(i + 1, j + 2));
  float g_0_90_max = max(g_0, g_90);
  float g_0_90_min = min(g_0, g_90);
  float g_45_135_max = max(g_45, g_135);
  float g_45_135_min = min(g_45, g_135);
  if (g_0_90_max + g_45_135_max == 0.0) return vec4(0.0);
  float e_0_90 = min(g_0_90_max / (g_0_90_max + g_45_135_max), 1.0);
  float e_45_135 = 1.0 - e_0_90;
  bool c_0_90 = (g_0_90_max > (g_0_90_min * kDetectRatio)) && (g_0_90_max > kDetectThres) && (g_0_90_max > g_45_135_min);
  bool c_45_135 = (g_45_135_max > (g_45_135_min * kDetectRatio)) && (g_45_135_max > kDetectThres) && (g_45_135_max > g_0_90_min);
  bool c_g_0_90 = g_0_90_max == g_0;
  bool c_g_45_135 = g_45_135_max == g_45;
  float f_e_0_90 = (c_0_90 && c_45_135) ? e_0_90 : 1.0;
  float f_e_45_135 = (c_0_90 && c_45_135) ? e_45_135 : 1.0;
  float weight_0 = (c_0_90 && c_g_0_90) ? f_e_0_90 : 0.0;
  float weight_90 = (c_0_90 && !c_g_0_90) ? f_e_0_90 : 0.0;
  float weight_45 = (c_45_135 && c_g_45_135) ? f_e_45_135 : 0.0;
  float weight_135 = (c_45_135 && !c_g_45_135) ? f_e_45_135 : 0.0;
  return vec4(weight_0, weight_90, weight_45, weight_135);
}

void coefScaler(int ph, out float c[6]) {
  vec4 a = texelFetch(u_coef, ivec2(0, ph), 0);
  vec4 b = texelFetch(u_coef, ivec2(1, ph), 0);
  c[0] = a.x; c[1] = a.y; c[2] = a.z; c[3] = a.w; c[4] = b.x; c[5] = b.y;
}
void coefUsm(int ph, out float c[6]) {
  vec4 a = texelFetch(u_coef, ivec2(2, ph), 0);
  vec4 b = texelFetch(u_coef, ivec2(3, ph), 0);
  c[0] = a.x; c[1] = a.y; c[2] = a.z; c[3] = a.w; c[4] = b.x; c[5] = b.y;
}

float calcLTI(float p0, float p1, float p2, float p3, float p4, float p5, int phase_index) {
  bool selector = (phase_index <= kPhaseCount / 2);
  float sel = selector ? p0 : p3;
  float a_min = min(min(p1, p2), sel);
  float a_max = max(max(p1, p2), sel);
  sel = selector ? p2 : p5;
  float b_min = min(min(p3, p4), sel);
  float b_max = max(max(p3, p4), sel);
  float a_cont = a_max - a_min;
  float b_cont = b_max - b_min;
  float cont_ratio = max(a_cont, b_cont) / (min(a_cont, b_cont) + kEps);
  return (1.0 - clamp((cont_ratio - kMinContrastRatio) * kRatioNorm, 0.0, 1.0)) * kContrastBoost;
}

float evalPoly6(float pxl[6], int phase_int) {
  float cs[6];
  float cu[6];
  coefScaler(phase_int, cs);
  coefUsm(phase_int, cu);
  float y = 0.0;
  float y_usm = 0.0;
  for (int i = 0; i < 6; ++i) {
    y += cs[i] * pxl[i];
    y_usm += cu[i] * pxl[i];
  }
  float y_scale = 1.0 - clamp((y - kSharpStartY) * kSharpScaleY, 0.0, 1.0);
  float y_sharpness = y_scale * kSharpStrengthScale + kSharpStrengthMin;
  y_usm *= y_sharpness;
  float y_sharpness_limit = (y_scale * kSharpLimitScale + kSharpLimitMin) * y;
  y_usm = min(y_sharpness_limit, max(-y_sharpness_limit, y_usm));
  y_usm *= calcLTI(pxl[0], pxl[1], pxl[2], pxl[3], pxl[4], pxl[5], phase_int);
  return y + y_usm;
}

float filterNormal(int phase_x, int phase_y) {
  float cx[6];
  float cy[6];
  coefScaler(phase_x, cx);
  coefScaler(phase_y, cy);
  float h_acc = 0.0;
  for (int j = 0; j < 6; ++j) {
    float v_acc = 0.0;
    for (int i = 0; i < 6; ++i) v_acc += px(i, j) * cy[i];
    h_acc += v_acc * cx[j];
  }
  return h_acc;
}

float addDirFilters(float fx, float fy, int fx_int, int fy_int, vec4 w) {
  float f = 0.0;
  float t[6];
  float tmp[7];
  if (w.x > 0.0) {  // 0 degrees
    for (int i = 0; i < 6; ++i) t[i] = mix(px(i, 2), px(i, 3), fx);
    f += evalPoly6(t, fy_int) * w.x;
  }
  if (w.y > 0.0) {  // 90 degrees
    for (int i = 0; i < 6; ++i) t[i] = mix(px(2, i), px(3, i), fy);
    f += evalPoly6(t, fx_int) * w.y;
  }
  if (w.z > 0.0) {  // 45 degrees
    float pb = 0.5 + 0.5 * (fx - fy);
    tmp[1] = mix(px(2, 1), px(1, 2), pb);
    tmp[3] = mix(px(3, 2), px(2, 3), pb);
    tmp[5] = mix(px(4, 3), px(3, 4), pb);
    pb = pb - 0.5;
    float a = (pb >= 0.0) ? px(0, 2) : px(2, 0);
    float b = (pb >= 0.0) ? px(1, 3) : px(3, 1);
    float c = (pb >= 0.0) ? px(2, 4) : px(4, 2);
    float d = (pb >= 0.0) ? px(3, 5) : px(5, 3);
    tmp[0] = mix(px(1, 1), a, abs(pb));
    tmp[2] = mix(px(2, 2), b, abs(pb));
    tmp[4] = mix(px(3, 3), c, abs(pb));
    tmp[6] = mix(px(4, 4), d, abs(pb));
    float pp = fx + fy;
    int off = 0;
    if (pp >= 1.0) {
      off = 1;
      pp -= 1.0;
    }
    for (int i = 0; i < 6; ++i) t[i] = tmp[i + off];
    f += evalPoly6(t, int(pp * 64.0)) * w.z;
  }
  if (w.w > 0.0) {  // 135 degrees
    float pb = 0.5 * (fx + fy);
    tmp[1] = mix(px(3, 1), px(4, 2), pb);
    tmp[3] = mix(px(2, 2), px(3, 3), pb);
    tmp[5] = mix(px(1, 3), px(2, 4), pb);
    pb = pb - 0.5;
    float a = (pb >= 0.0) ? px(5, 2) : px(3, 0);
    float b = (pb >= 0.0) ? px(4, 3) : px(2, 1);
    float c = (pb >= 0.0) ? px(3, 4) : px(1, 2);
    float d = (pb >= 0.0) ? px(2, 5) : px(0, 3);
    tmp[0] = mix(px(4, 1), a, abs(pb));
    tmp[2] = mix(px(3, 2), b, abs(pb));
    tmp[4] = mix(px(2, 3), c, abs(pb));
    tmp[6] = mix(px(1, 4), d, abs(pb));
    float pp = 1.0 + (fx - fy);
    int off = 0;
    if (pp >= 1.0) {
      off = 1;
      pp -= 1.0;
    }
    for (int i = 0; i < 6; ++i) t[i] = tmp[i + off];
    f += evalPoly6(t, int(pp * 64.0)) * w.w;
  }
  return f;
}

void main() {
  vec2 src = (floor(gl_FragCoord.xy) + 0.5) * u_scale - 0.5;
  vec2 fl = floor(src);
  vec2 fr = src - fl;
  ivec2 base = ivec2(fl) - 2;
  for (int r = 0; r < 6; ++r)
    for (int c = 0; c < 6; ++c)
      P[r * 6 + c] = getY(texelFetch(u_video, clamp(base + ivec2(c, r), ivec2(0), u_max), 0).rgb);
  int fx_int = int(fr.x * float(kPhaseCount));
  int fy_int = int(fr.y * float(kPhaseCount));
  vec4 e00 = edgeMap(1, 1), e01 = edgeMap(1, 2), e10 = edgeMap(2, 1), e11 = edgeMap(2, 2);
  vec4 w = mix(mix(e00, e01, fr.x), mix(e10, e11, fr.x), fr.y);
  float baseWeight = 1.0 - w.x - w.y - w.z - w.w;
  float opY = filterNormal(fx_int, fy_int) * baseWeight;
  opY += addDirFilters(fr.x, fr.y, fx_int, fy_int, w);
  vec3 op = texture(u_video, v_uv).rgb;
  op += vec3(opY - getY(op));
  o = vec4(clamp(op, 0.0, 1.0), 1.0);
}
)";

// CRT look, drawn on the finished picture (after any upscaler and after the
// picture controls; no gamma change):
//   - barrel curvature of the glass, black outside it, soft vignette;
//   - horizontal scanlines: each line is a beam whose width grows with the
//     brightness, so dark areas show deep gaps and bright areas almost none;
//   - shadow mask: R, G, B slots in a staggered 2 x 2 cell (every other
//     slot column is shifted down by half a slot), mask depth 0.80.
// u_triad is the width of one R+G+B slot group in output pixels and u_lines
// the number of scanlines over the picture height; runRetro() derives both
// from the size the picture is shown at, so the pattern never drops below
// the display's pixel pitch.
const char* kCrtFS = R"(#version 330 core
uniform sampler2D u_video;
uniform vec2 u_outSize;
uniform float u_triad;
uniform float u_lines;
in vec2 v_uv;
out vec4 o;
const float kMaskDepth = 0.80;
const float kBend = 0.055;
const float kMaskGain = 2.1;
vec2 bend(vec2 uv) {
  vec2 p = uv * 2.0 - 1.0;
  p *= 1.0 + kBend * dot(p, p);
  return p * 0.5 + 0.5;
}
vec3 slotMask(vec2 px) {
  float group = floor(px.x / u_triad);
  float phase = fract(px.x / u_triad) * 3.0;
  float slot = fract(px.y / (u_triad * 2.0) + 0.5 * mod(group, 2.0));
  float open = smoothstep(0.0, 0.12, slot) * smoothstep(1.0, 0.88, slot);
  vec3 phosphor = vec3(step(phase, 1.0), step(1.0, phase) * step(phase, 2.0), step(2.0, phase));
  return mix(vec3(1.0), phosphor * mix(0.55, 1.0, open), kMaskDepth);
}
void main() {
  vec2 uv = bend(v_uv);
  vec2 edge = min(uv, 1.0 - uv);
  if (edge.x < 0.0 || edge.y < 0.0) {
    o = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }
  vec3 c = texture(u_video, uv).rgb;
  float y = dot(c, vec3(0.2126, 0.7152, 0.0722));
  float d = abs(fract(uv.y * u_lines) - 0.5);
  float beam = mix(0.22, 0.50, sqrt(y));
  float scan = exp(-(d * d) / (2.0 * beam * beam));
  vec3 lit = c * scan * slotMask(v_uv * u_outSize) * kMaskGain;
  lit = mix(lit, c, 0.18 * y);
  float px = 3.0 / max(u_outSize.y, 1.0);
  float glass = smoothstep(0.0, px, edge.x) * smoothstep(0.0, px, edge.y);
  float vignette = pow(clamp(16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y), 0.0, 1.0), 0.18);
  o = vec4(clamp(lit, 0.0, 1.0) * vignette * glass, 1.0);
}
)";

// VHS look, drawn on the finished picture: a home video tape played back on
// a television.
//   - colour is split into luma and chroma (YIQ): luma is softened a little,
//     chroma is blurred much wider and lands slightly to the right, so
//     colours smear past the edges they belong to;
//   - mild desaturation, a warm cast and slightly raised blacks;
//   - grain that changes every frame, faint chroma noise and the odd white
//     dropout streak;
//   - every line wobbles by a fraction of a pixel; the bottom few percent
//     ("head switching") bend sideways by a few pixels;
//   - now and then a faint tracking band drifts down the picture;
//   - a soft vignette towards the edges.
// Distances are in pixels of a 640-wide tape raster so the look is the same
// at any output size. u_time drives the animation (seconds).
const char* kVhsFS = R"(#version 330 core
uniform sampler2D u_video;
uniform float u_time;
uniform float u_lines;
in vec2 v_uv;
out vec4 o;
const float kPx = 1.0 / 640.0;
float hash12(vec2 p) {
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}
vec3 toYiq(vec3 c) {
  return vec3(dot(c, vec3(0.299, 0.587, 0.114)), dot(c, vec3(0.596, -0.274, -0.322)),
              dot(c, vec3(0.211, -0.523, 0.312)));
}
vec3 toRgb(vec3 q) {
  return vec3(q.x + 0.956 * q.y + 0.621 * q.z, q.x - 0.272 * q.y - 0.647 * q.z, q.x - 1.106 * q.y + 1.703 * q.z);
}
void main() {
  float frame = floor(u_time * 30.0);
  float line = floor(v_uv.y * u_lines);
  vec2 uv = v_uv;
  // Line wobble everywhere (a fraction of a pixel) ...
  float shift = (hash12(vec2(line, frame)) - 0.5) * 0.7 * kPx;
  // ... and head switching at the bottom: lines bend to the right.
  float hs = smoothstep(0.955, 1.0, v_uv.y);
  shift += hs * (2.5 + 3.0 * hash12(vec2(line, frame + 7.0))) * kPx;
  // Tracking band: every 11 s cycle has a chance of a band crossing the
  // picture from top to bottom.
  float cyc = u_time / 11.0;
  float bandOn = step(0.4, hash12(vec2(floor(cyc), 3.7)));
  float bandY = fract(cyc) * 1.4 - 0.2;
  float bd = (v_uv.y - bandY) / 0.03;
  float band = exp(-bd * bd) * bandOn;
  shift += band * (hash12(vec2(line, frame + 3.0)) - 0.35) * 5.0 * kPx;
  uv.x -= shift;
  // Luma: softened (5 taps over ±1.5 tape pixels).
  float y = 0.0;
  y += toYiq(texture(u_video, uv + vec2(-1.5 * kPx, 0.0)).rgb).x * 0.10;
  y += toYiq(texture(u_video, uv + vec2(-0.75 * kPx, 0.0)).rgb).x * 0.22;
  y += toYiq(texture(u_video, uv).rgb).x * 0.36;
  y += toYiq(texture(u_video, uv + vec2(0.75 * kPx, 0.0)).rgb).x * 0.22;
  y += toYiq(texture(u_video, uv + vec2(1.5 * kPx, 0.0)).rgb).x * 0.10;
  // Chroma: wide (6 taps over 10 tape pixels), centred 2.5 pixels to the left
  // of the pixel so colour trails to the right.
  vec2 iq = vec2(0.0);
  for (int i = 0; i < 6; ++i) iq += toYiq(texture(u_video, uv + vec2((float(i) * 2.0 - 7.5) * kPx, 0.0)).rgb).yz;
  iq /= 6.0;
  // Chroma noise in wide streaks, and slightly reduced saturation.
  vec2 cell = vec2(floor(uv.x / (8.0 * kPx)), line);
  iq += (vec2(hash12(cell + frame), hash12(cell + frame + 19.0)) - 0.5) * 0.035;
  iq *= 0.8;
  vec3 c = toRgb(vec3(y, iq));
  // Warm cast, lifted blacks, softer highlights.
  c = c * vec3(1.05, 1.0, 0.90) + vec3(0.03, 0.022, 0.012);
  c = c / (1.0 + 0.12 * c);
  // Grain, changing every frame.
  vec2 g = floor(v_uv * vec2(960.0, 540.0));
  float n = hash12(g + vec2(frame * 17.0, frame * 31.0)) - 0.5;
  c += n * (0.05 + 0.10 * hs);
  // Tracking band: brighter, with noisy streaks.
  float streak = step(0.85, hash12(vec2(floor(v_uv.x / (20.0 * kPx)), line + frame * 13.0)));
  c += band * (0.05 + 0.3 * streak * hash12(vec2(line, frame)));
  // Dropouts: a rare short white streak.
  float dl = hash12(vec2(line, frame + 41.0));
  float dx = abs(v_uv.x - hash12(vec2(line, frame + 59.0))) / 0.025;
  c = mix(c, vec3(0.92), step(0.9975, dl) * clamp(1.0 - dx, 0.0, 1.0) * 0.8);
  // Head switching drags in black from the left edge.
  if (uv.x < 0.0) c = vec3(0.02);
  float v = 16.0 * v_uv.x * v_uv.y * (1.0 - v_uv.x) * (1.0 - v_uv.y);
  c *= mix(0.78, 1.0, pow(clamp(v, 0.0, 1.0), 0.35));
  o = vec4(clamp(c, 0.0, 1.0), 1.0);
}
)";

// Arcade look, drawn on the finished picture: an arcade cabinet's
// low-resolution monitor.
//   - aperture grille: continuous vertical R, G, B phosphor stripes (no slot
//     mask), u_triad output pixels per stripe group;
//   - strong scanlines: the picture is sampled once per line and each line is
//     a beam whose width grows with brightness, so dark areas show deep gaps;
//   - a soft glow from a few wide taps around the pixel (bright areas bloom);
//   - punchier colour (more saturation and contrast);
//   - slight curvature and rounded, dark corners.
const char* kArcadeFS = R"(#version 330 core
uniform sampler2D u_video;
uniform vec2 u_outSize;
uniform float u_triad;
uniform float u_lines;
in vec2 v_uv;
out vec4 o;
const float kBend = 0.03;
const float kGrille = 0.72;
vec2 bend(vec2 uv) {
  vec2 p = uv * 2.0 - 1.0;
  p *= 1.0 + kBend * dot(p, p);
  return p * 0.5 + 0.5;
}
// Brightness of the phosphor stripe centred at `centre` (0.5, 1.5, 2.5) at
// `phase` (0 … 3 across one R+G+B group, wrapping around).
float stripeAt(float phase, float centre) {
  float d = abs(phase - centre);
  d = min(d, 3.0 - d);
  return clamp((0.75 - d) * 2.5, 0.0, 1.0);
}
vec3 punch(vec3 c) {
  float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
  c = max(mix(vec3(l), c, 1.3), 0.0);
  return clamp((c - 0.5) * 1.12 + 0.5, 0.0, 1.0);
}
void main() {
  vec2 uv = bend(v_uv);
  float aspect = u_outSize.x / max(u_outSize.y, 1.0);
  // Rounded screen edge (signed distance, in picture heights).
  vec2 q = abs(uv - 0.5) * vec2(aspect, 1.0);
  vec2 hb = vec2(0.5 * aspect, 0.5);
  float r = 0.07;
  float d = length(max(q - hb + r, 0.0)) + min(max(q.x - hb.x + r, q.y - hb.y + r), 0.0) - r;
  float px = 1.5 / max(u_outSize.y, 1.0);
  float screen = 1.0 - smoothstep(-px, px, d);
  if (screen <= 0.0) {
    o = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }
  // One sample per scanline (low-resolution source), beam around it.
  float ly = uv.y * u_lines;
  vec2 suv = vec2(uv.x, (floor(ly) + 0.5) / u_lines);
  vec3 c = punch(texture(u_video, suv).rgb);
  float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
  float dy = fract(ly) - 0.5;
  float beam = mix(0.16, 0.40, sqrt(lum));
  float scan = exp(-(dy * dy) / (2.0 * beam * beam));
  // Aperture grille: vertical stripes with soft edges.
  float phase = fract(v_uv.x * u_outSize.x / u_triad) * 3.0;
  vec3 stripe = vec3(stripeAt(phase, 0.5), stripeAt(phase, 1.5), stripeAt(phase, 2.5));
  vec3 mask = mix(vec3(1.0), stripe, kGrille);
  vec3 lit = c * scan * mask * 2.0;
  // Glow: eight wide taps.
  vec2 s1 = vec2(0.006 / aspect, 0.006), s2 = vec2(0.016 / aspect, 0.016);
  vec3 glow = texture(u_video, uv + vec2(s1.x, s1.y)).rgb + texture(u_video, uv + vec2(-s1.x, s1.y)).rgb +
              texture(u_video, uv + vec2(s1.x, -s1.y)).rgb + texture(u_video, uv + vec2(-s1.x, -s1.y)).rgb;
  glow += texture(u_video, uv + vec2(s2.x, 0.0)).rgb + texture(u_video, uv - vec2(s2.x, 0.0)).rgb +
          texture(u_video, uv + vec2(0.0, s2.y)).rgb + texture(u_video, uv - vec2(0.0, s2.y)).rgb;
  glow = punch(glow / 8.0);
  lit += glow * glow * 0.32;
  // Corners and edges fall off into the bezel.
  float edge = smoothstep(0.0, 0.06, -d);
  float vignette = pow(clamp(16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y), 0.0, 1.0), 0.22);
  o = vec4(clamp(lit, 0.0, 1.0) * vignette * mix(0.55, 1.0, edge) * screen, 1.0);
}
)";

// 9-tap Gaussian along u_dir (texel step), with a saturation boost — used
// for the blurred "ambient" backdrop behind the Liquid Glass UI.
const char* kBlurFS = R"(#version 330 core
uniform sampler2D u_tex;
uniform vec2 u_dir;
uniform float u_sat;
in vec2 v_uv;
out vec4 o;
void main() {
  const float w[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
  vec3 c = texture(u_tex, v_uv).rgb * w[0];
  for (int i = 1; i < 5; ++i) {
    c += texture(u_tex, v_uv + u_dir * float(i)).rgb * w[i];
    c += texture(u_tex, v_uv - u_dir * float(i)).rgb * w[i];
  }
  float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
  o = vec4(clamp(mix(vec3(l), c, u_sat), 0.0, 1.0), 1.0);
}
)";

const char* kBlitFS = R"(#version 330 core
uniform sampler2D u_tex;
in vec2 v_uv;
out vec4 o;
void main() { o = vec4(texture(u_tex, v_uv).rgb, 1.0); }
)";

GLuint compile(GLenum type, const char* src0) {
  std::string code = src0;
  const char* src = code.c_str();
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetShaderInfoLog(s, sizeof log, nullptr, log);
    std::fprintf(stderr, "shader compile error: %s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

}  // namespace

GLuint linkProgram(const char* fs) {
  GLuint v = compile(GL_VERTEX_SHADER, kVS);
  GLuint f = compile(GL_FRAGMENT_SHADER, fs);
  if (!v || !f) return 0;
  GLuint p = glCreateProgram();
  glAttachShader(p, v);
  glAttachShader(p, f);
  glLinkProgram(p);
  glDeleteShader(v);
  glDeleteShader(f);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetProgramInfoLog(p, sizeof log, nullptr, log);
    std::fprintf(stderr, "program link error: %s\n", log);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

namespace {

void setPictureUniforms(GLuint prog, const ColorAdjust& c) {
  glUniform1f(glGetUniformLocation(prog, "u_gain"), c.brightness);
  glUniform1f(glGetUniformLocation(prog, "u_contrast_gain"), c.contrast);
  glUniform1f(glGetUniformLocation(prog, "u_sat_gain"), c.saturation);
  glUniform1f(glGetUniformLocation(prog, "u_gamma"), std::clamp(c.gamma, 0.6f, 1.4f));
}

void setTexParams(GLenum filter) {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}  // namespace

// Saves/restores the bits of GL state our passes touch so Dear ImGui's
// renderer is unaffected.
GlStateGuard::GlStateGuard() {
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
  glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
  glGetIntegerv(GL_VIEWPORT, viewport);
  blend = glIsEnabled(GL_BLEND);
  scissor = glIsEnabled(GL_SCISSOR_TEST);
  depth = glIsEnabled(GL_DEPTH_TEST);
  cull = glIsEnabled(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}

GlStateGuard::~GlStateGuard() {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glUseProgram(prog);
  glBindVertexArray(vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glActiveTexture(active);
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  if (blend) glEnable(GL_BLEND);
  if (scissor) glEnable(GL_SCISSOR_TEST);
  if (depth) glEnable(GL_DEPTH_TEST);
  if (cull) glEnable(GL_CULL_FACE);
}

void GpuImage::ensure(int nw, int nh, GLenum ifmt) {
  if (tex && nw == w && nh == h && ifmt == fmt) return;
  if (!tex) glGenTextures(1, &tex);
  if (!fbo) glGenFramebuffers(1, &fbo);
  w = nw;
  h = nh;
  fmt = ifmt;
  GLint prevTex;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
  glBindTexture(GL_TEXTURE_2D, tex);
  if (ifmt == GL_RGBA16F)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  setTexParams(GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, prevTex);
  GLint prevFbo;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

void GpuImage::release() {
  if (fbo) glDeleteFramebuffers(1, &fbo);
  if (tex) glDeleteTextures(1, &tex);
  fbo = tex = 0;
  w = h = 0;
  fmt = 0;
}

// Anime4K CNN x2 (MIT, Copyright (c) 2019-2021 bloc97), converted from the
// mpv shaders by tools/gen_anime4k.py. M (7 conv layers) on desktop; S
// (3 layers) on Quest, where the GPU also renders the VR scene.
namespace {
struct Anime4KPass {
  const char* save;
  int scale;
  int nbind;
  const char* binds[7];
  const char* fs;
};
#include "Anime4KData.inc"
#include "NISCoef.inc"
const auto& kAnime4KNet = kAnime4K_M;
}  // namespace

// Intermediate network activations (signed, so half-float), by name.
struct Anime4KTextures {
  Anime4KTextures() { tex.reserve(16); }  // references stay valid
  std::vector<std::pair<std::string, GpuImage>> tex;
  GpuImage& get(const std::string& n) {
    for (auto& t : tex)
      if (t.first == n) return t.second;
    tex.emplace_back(n, GpuImage{});
    return tex.back().second;
  }
  ~Anime4KTextures() {
    for (auto& t : tex) t.second.release();
  }
};

GpuPrograms& GpuPrograms::get() {
  static GpuPrograms g;
  return g;
}

bool GpuPrograms::init() {
  convert = linkProgram(kConvertFS);
  bcs = linkProgram(kBcsFS);
  easu = linkProgram(kEasuFS);
  rcas = linkProgram(kRcasFS);
  lanczos = linkProgram(kLanczosFS);
  cas = linkProgram(kCasFS);
  nis = linkProgram(kNisFS);
  {
    // NIS filter banks as a small float texture (texelFetch only).
    glGenTextures(1, &nisCoef);
    GLint prev;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    glBindTexture(GL_TEXTURE_2D, nisCoef);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 64, 0, GL_RGBA, GL_FLOAT, kNisCoef);
    setTexParams(GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, prev);
  }
  crt = linkProgram(kCrtFS);
  vhs = linkProgram(kVhsFS);
  arcade = linkProgram(kArcadeFS);
  blit = linkProgram(kBlitFS);
  blur = linkProgram(kBlurFS);
  glGenVertexArrays(1, &vao);
  return convert && bcs && easu && rcas && lanczos && cas && nis && crt && vhs && arcade && blit && blur;
}

const std::vector<GLuint>& GpuPrograms::anime4k() {
  if (anime4kState != 0) return a4k;
  anime4kState = -1;
  for (const Anime4KPass& p : kAnime4KNet) {
    GLuint prog = linkProgram(p.fs);
    if (!prog) {
      for (GLuint q : a4k) glDeleteProgram(q);
      a4k.clear();
      return a4k;
    }
    a4k.push_back(prog);
  }
  anime4kState = 1;
  return a4k;
}

void GpuPrograms::shutdown() {
  for (GLuint* p : {&convert, &bcs, &easu, &rcas, &lanczos, &cas, &nis, &crt, &vhs, &arcade, &blit, &blur})
    if (*p) glDeleteProgram(*p), *p = 0;
  if (nisCoef) glDeleteTextures(1, &nisCoef), nisCoef = 0;
  for (GLuint q : a4k) glDeleteProgram(q);
  a4k.clear();
  anime4kState = 0;
  if (vao) glDeleteVertexArrays(1, &vao);
  vao = 0;
}

void GpuPrograms::quad(GLuint prog, int W, int H, float x0, float y0, float x1, float y1, float u0, float v0,
                       float u1, float v1) {
  glUniform4f(glGetUniformLocation(prog, "u_dst"), -1.f + 2.f * x0 / W, -1.f + 2.f * y0 / H, -1.f + 2.f * x1 / W,
              -1.f + 2.f * y1 / H);
  glUniform4f(glGetUniformLocation(prog, "u_uvr"), u0, v0, u1, v1);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

VideoPipeline::~VideoPipeline() { release(); }

void VideoPipeline::release() {
  for (int i = 0; i < 3; ++i)
    if (planes_[i]) glDeleteTextures(1, &planes_[i]), planes_[i] = 0, planeW_[i] = planeH_[i] = 0;
  src_.release();
  fx_.release();
  easu_.release();
  a4kOut_.release();
  lzTmp_.release();
  lzOut_.release();
  nisOut_.release();
  crtOut_.release();
  delete a4kTex_;
  a4kTex_ = nullptr;
  depthFx_.release();
  delete depth_;
  depth_ = nullptr;
  viewPtr_ = fxOut_ = &src_;
  haveFx_ = false;
}

void VideoPipeline::upload(const Frame& f) {
  if (f.width <= 0 || f.height <= 0) return;
  GlStateGuard guard;
  auto& P = GpuPrograms::get();
  int fmtCode = 0, swap = 0, csx = 0, csy = 0;
  struct Up {
    int w, h;
    GLenum ifmt, fmt;
    int bpp;
  } ups[3] = {};
  int nplanes = 1;
  const int W = f.width, H = f.height;
  switch (f.fmt) {
    case PixFmt::YUYV:
    case PixFmt::UYVY:
      fmtCode = f.fmt == PixFmt::YUYV ? 0 : 1;
      ups[0] = {W / 2, H, GL_RGBA8, GL_RGBA, 4};
      break;
    case PixFmt::NV12:
    case PixFmt::NV21:
      fmtCode = 2;
      swap = f.fmt == PixFmt::NV21;
      nplanes = 2;
      ups[0] = {W, H, GL_R8, GL_RED, 1};
      ups[1] = {(W + 1) / 2, (H + 1) / 2, GL_RG8, GL_RG, 2};
      break;
    case PixFmt::I420:
    case PixFmt::YUV422P:
    case PixFmt::YUV444P:
      fmtCode = 3;
      nplanes = 3;
      csx = f.fmt == PixFmt::YUV444P ? 0 : 1;
      csy = f.fmt == PixFmt::I420 ? 1 : 0;
      ups[0] = {W, H, GL_R8, GL_RED, 1};
      ups[1] = ups[2] = {(W + csx) >> csx, (H + csy) >> csy, GL_R8, GL_RED, 1};
      break;
    case PixFmt::RGB24:
    case PixFmt::BGR24:
      fmtCode = 4;
      swap = f.fmt == PixFmt::BGR24;
      ups[0] = {W, H, GL_RGB8, GL_RGB, 3};
      break;
    case PixFmt::RGBA:
    case PixFmt::BGRA:
      fmtCode = 4;
      ups[0] = {W, H, GL_RGBA8, (GLenum)(f.fmt == PixFmt::BGRA ? GL_BGRA : GL_RGBA), 4};
      break;
    case PixFmt::GRAY8:
      fmtCode = 5;
      ups[0] = {W, H, GL_R8, GL_RED, 1};
      break;
    default: return;
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  for (int i = 0; i < nplanes; ++i) {
    if (!f.plane[i]) return;
    if (!planes_[i]) glGenTextures(1, &planes_[i]);
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, planes_[i]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, f.stride[i] / ups[i].bpp);
    if (planeW_[i] != ups[i].w || planeH_[i] != ups[i].h || planeFmt_[i] != ups[i].ifmt) {
      glTexImage2D(GL_TEXTURE_2D, 0, ups[i].ifmt, ups[i].w, ups[i].h, 0, ups[i].fmt, GL_UNSIGNED_BYTE, f.plane[i]);
      setTexParams(i == 0 ? GL_NEAREST : GL_LINEAR);
      planeW_[i] = ups[i].w;
      planeH_[i] = ups[i].h;
      planeFmt_[i] = ups[i].ifmt;
    } else {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ups[i].w, ups[i].h, ups[i].fmt, GL_UNSIGNED_BYTE, f.plane[i]);
    }
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  src_.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, src_.fbo);
  glViewport(0, 0, W, H);
  GLuint prog = P.convert;
  glUseProgram(prog);
  glUniform1i(glGetUniformLocation(prog, "t0"), 0);
  glUniform1i(glGetUniformLocation(prog, "t1"), 1);
  glUniform1i(glGetUniformLocation(prog, "t2"), 2);
  glUniform1i(glGetUniformLocation(prog, "u_fmt"), fmtCode);
  glUniform1i(glGetUniformLocation(prog, "u_swap"), swap);
  glUniform2f(glGetUniformLocation(prog, "u_size"), (float)W, (float)H);
  glUniform2i(glGetUniformLocation(prog, "u_csub"), csx, csy);
  // YUV → RGB matrix (column-major) + offsets.
  float kr, kb;
  if (f.bt709) {
    kr = 0.2126f;
    kb = 0.0722f;
  } else {
    kr = 0.299f;
    kb = 0.114f;
  }
  float kg = 1.f - kr - kb;
  float ys = f.fullRange ? 1.f : 255.f / 219.f;
  float cs = f.fullRange ? 1.f : 255.f / 224.f;
  float rCr = 2.f * (1.f - kr);
  float bCb = 2.f * (1.f - kb);
  float gCb = 2.f * (1.f - kb) * kb / kg;
  float gCr = 2.f * (1.f - kr) * kr / kg;
  float m[9] = {ys, ys, ys, 0.f, -gCb * cs, bCb * cs, rCr * cs, -gCr * cs, 0.f};
  glUniformMatrix3fv(glGetUniformLocation(prog, "u_m"), 1, GL_FALSE, m);
  float yoff = f.fullRange ? 0.f : 16.f / 255.f;
  glUniform3f(glGetUniformLocation(prog, "u_off"), yoff, 128.f / 255.f, 128.f / 255.f);
  P.quad(prog, W, H, 0, 0, (float)W, (float)H);
  glActiveTexture(GL_TEXTURE0);
}

void VideoPipeline::process(Effect upscaler, const ColorAdjust& c, int retro, float depth, bool newFrame) {
  if (!hasImage()) return;
  // The passes below bind their own framebuffers, programs and viewports;
  // the caller's (the window's default framebuffer, which the interface is
  // drawn into next) must be bound again afterwards.
  GlStateGuard guard;
  if (retro < RetroOff || retro > RetroArcade) retro = RetroOff;
  // Upscaling, colour and the TV style only depend on the frame and the
  // settings, so repeated display frames reuse the previous result (the VHS
  // noise moves on with every new frame).
  Key k;
  k.e = upscaler;
  k.retro = retro;
  k.c = c;
  k.w = src_.w;
  k.h = src_.h;
  k.tex = src_.tex;
  if (retro != RetroOff) k.dw = dispW_, k.dh = dispH_;
  if (isUpscale(upscaler)) k.lw = limW_, k.lh = limH_;
  if (newFrame || !haveFx_ || !(k == lastKey_)) {
    runEffect(upscaler, c);
    if (retro != RetroOff) runRetro(retro, *fxOut_);
    lastKey_ = k;
    haveFx_ = true;
  }
  viewPtr_ = fxOut_;
  if (depth > 0.f) {
    if (!depth_) depth_ = new DepthEngine();
    // Depth is estimated on the clean picture (before the TV style / sharpening).
    if (newFrame || !depth_->ready()) depth_->update(src_);
    depth_->enhance(*fxOut_, depthFx_, depth);
    if (depth_->ready()) viewPtr_ = &depthFx_;
  } else if (depth_ && depth_->ready()) {
    depth_->reset();
  }
}

// 4K target: the largest uniform scale that fits maxW × maxH, at most 4×;
// below 1.05× the picture is left at its own size.
void VideoPipeline::upscaleSize(int w, int h, int& ow, int& oh, int maxW, int maxH) {
  ow = w;
  oh = h;
  if (w <= 0 || h <= 0) return;
  double scale = std::min({(double)maxW / w, (double)maxH / h, 4.0});
  if (scale < 1.05) return;
  ow = std::min(maxW, (int)std::lround(w * scale));
  oh = std::min(maxH, (int)std::lround(h * scale));
}

// Upscaler (when one is chosen and the target is larger than the source)
// with the picture controls folded into its last pass; otherwise just the
// picture controls at source size. Result in fxOut_.
void VideoPipeline::runEffect(Effect upscaler, const ColorAdjust& c) {
  int W = src_.w, H = src_.h;
  if (isUpscale(upscaler)) upscaleSize(src_.w, src_.h, W, H, limW_, limH_);
  bool scaled = W != src_.w || H != src_.h;
  if (scaled) {
    switch (upscaler) {
      case Effect::Anime4K:
        if (runAnime4K(W, H, c)) return;
        runFsr(src_, W, H, c);  // network unavailable on this GPU
        return;
      case Effect::LanczosCas: runLanczosCas(W, H, c); return;
      case Effect::Nis: runNis(W, H, c); return;
      default: runFsr(src_, W, H, c); return;
    }
  }
  if (c.identity())
    fxOut_ = &src_;
  else
    runColor(src_, W, H, c);
}

// TV-style pass (CRT, VHS or Arcade) over the finished picture `in`, at its
// own size, into crtOut_.
void VideoPipeline::runRetro(int style, const GpuImage& in) {
  auto& P = GpuPrograms::get();
  GLuint prog = style == RetroVhs ? P.vhs : style == RetroArcade ? P.arcade : P.crt;
  if (!prog) return;
  int W = in.w, H = in.h;
  // Output pixels per display pixel (≥ 1): the patterns are laid out in
  // display pixels so they stay crisp however the picture is scaled.
  float perDisp = dispW_ > 0 ? std::max(1.f, (float)W / dispW_) : 1.f;
  float triad = 3.f * perDisp;
  int shownH = dispH_ > 0 ? std::min(dispH_, H) : H;
  float lines;
  if (style == RetroArcade)  // a low-resolution arcade raster: 120 … 270 lines, ≥ 4.5 display rows each
    lines = std::clamp(shownH / 4.5f, 120.f, 270.f);
  else if (style == RetroVhs)  // tape lines (wobble and noise rows)
    lines = 480.f;
  else  // CRT: one scanline per 3 display rows at least, never more than the picture has
    lines = std::max(120.f, std::min((float)H / perDisp, shownH / 3.f));
  crtOut_.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, crtOut_.fbo);
  glViewport(0, 0, W, H);
  glUseProgram(prog);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, in.tex);
  glUniform1i(glGetUniformLocation(prog, "u_video"), 0);
  glUniform2f(glGetUniformLocation(prog, "u_outSize"), (float)W, (float)H);
  glUniform1f(glGetUniformLocation(prog, "u_triad"), triad);
  glUniform1f(glGetUniformLocation(prog, "u_lines"), lines);
  glUniform1f(glGetUniformLocation(prog, "u_time"), (float)std::fmod(time_, 3600.0));
  P.quad(prog, W, H, 0, 0, (float)W, (float)H);
  fxOut_ = &crtOut_;
}

// Plain resample + colour adjustments of `in` into fx_ (W × H).
void VideoPipeline::runColor(const GpuImage& in, int W, int H, const ColorAdjust& c) {
  auto& P = GpuPrograms::get();
  fx_.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, fx_.fbo);
  glViewport(0, 0, W, H);
  glUseProgram(P.bcs);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, in.tex);
  glUniform1i(glGetUniformLocation(P.bcs, "u_video"), 0);
  setPictureUniforms(P.bcs, c);
  P.quad(P.bcs, W, H, 0, 0, (float)W, (float)H);
  fxOut_ = &fx_;
}

// FSR 1: EASU from `in` to W × H (skipped at 1:1), then RCAS + colour into fx_.
void VideoPipeline::runFsr(const GpuImage& in, int W, int H, const ColorAdjust& c) {
  auto& P = GpuPrograms::get();
  const GpuImage* sharpenIn = &in;
  if (W != in.w || H != in.h) {
    easu_.ensure(W, H);
    glBindFramebuffer(GL_FRAMEBUFFER, easu_.fbo);
    glViewport(0, 0, W, H);
    glUseProgram(P.easu);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, in.tex);
    glUniform1i(glGetUniformLocation(P.easu, "u_video"), 0);
    // FsrEasuCon: output pixel → input pixel position.
    float sx = (float)in.w / W, sy = (float)in.h / H;
    glUniform4f(glGetUniformLocation(P.easu, "u_con0"), sx, sy, 0.5f * sx - 0.5f, 0.5f * sy - 0.5f);
    glUniform2i(glGetUniformLocation(P.easu, "u_max"), in.w - 1, in.h - 1);
    P.quad(P.easu, W, H, 0, 0, (float)W, (float)H);
    sharpenIn = &easu_;
  }
  fx_.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, fx_.fbo);
  glViewport(0, 0, W, H);
  glUseProgram(P.rcas);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sharpenIn->tex);
  glUniform1i(glGetUniformLocation(P.rcas, "u_video"), 0);
  // FsrRcasCon(0.15 stops): close to FSR's strongest setting.
  glUniform1f(glGetUniformLocation(P.rcas, "u_sharp"), std::exp2(-0.15f));
  glUniform2i(glGetUniformLocation(P.rcas, "u_max"), W - 1, H - 1);
  setPictureUniforms(P.rcas, c);
  P.quad(P.rcas, W, H, 0, 0, (float)W, (float)H);
  fxOut_ = &fx_;
}

void VideoPipeline::lanczos(const GpuImage& in, int W, int H, GpuImage& out) {
  auto& P = GpuPrograms::get();
  glUseProgram(P.lanczos);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(P.lanczos, "u_video"), 0);
  // Horizontal: in (w × h) → lzTmp_ (W × h).
  lzTmp_.ensure(W, in.h);
  glBindFramebuffer(GL_FRAMEBUFFER, lzTmp_.fbo);
  glViewport(0, 0, W, in.h);
  glBindTexture(GL_TEXTURE_2D, in.tex);
  glUniform2i(glGetUniformLocation(P.lanczos, "u_max"), in.w - 1, in.h - 1);
  glUniform1f(glGetUniformLocation(P.lanczos, "u_scale"), (float)in.w / W);
  glUniform1i(glGetUniformLocation(P.lanczos, "u_axis"), 0);
  P.quad(P.lanczos, W, in.h, 0, 0, (float)W, (float)in.h);
  // Vertical: lzTmp_ → out (W × H).
  out.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);
  glViewport(0, 0, W, H);
  glBindTexture(GL_TEXTURE_2D, lzTmp_.tex);
  glUniform2i(glGetUniformLocation(P.lanczos, "u_max"), W - 1, in.h - 1);
  glUniform1f(glGetUniformLocation(P.lanczos, "u_scale"), (float)in.h / H);
  glUniform1i(glGetUniformLocation(P.lanczos, "u_axis"), 1);
  P.quad(P.lanczos, W, H, 0, 0, (float)W, (float)H);
}

// Lanczos-3 to W × H, then CAS (sharpness 0.6) + colour into fx_.
void VideoPipeline::runLanczosCas(int W, int H, const ColorAdjust& c) {
  auto& P = GpuPrograms::get();
  const GpuImage* in = &src_;
  if (W != src_.w || H != src_.h) {
    lanczos(src_, W, H, lzOut_);
    in = &lzOut_;
  }
  fx_.ensure(W, H);
  glBindFramebuffer(GL_FRAMEBUFFER, fx_.fbo);
  glViewport(0, 0, W, H);
  glUseProgram(P.cas);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, in->tex);
  glUniform1i(glGetUniformLocation(P.cas, "u_video"), 0);
  glUniform2i(glGetUniformLocation(P.cas, "u_max"), W - 1, H - 1);
  const float sharpness = 0.6f;
  glUniform1f(glGetUniformLocation(P.cas, "u_peak"), -1.f / (8.f + (5.f - 8.f) * sharpness));
  setPictureUniforms(P.cas, c);
  P.quad(P.cas, W, H, 0, 0, (float)W, (float)H);
  fxOut_ = &fx_;
}

// NIS scales up to 2× (its supported range); above that (SD sources) the
// rest of the way to W × H is Lanczos-3. Colour adjustments in a final pass.
void VideoPipeline::runNis(int W, int H, const ColorAdjust& c) {
  auto& P = GpuPrograms::get();
  int NW = std::min(W, src_.w * 2), NH = std::min(H, src_.h * 2);
  nisOut_.ensure(NW, NH);
  glBindFramebuffer(GL_FRAMEBUFFER, nisOut_.fbo);
  glViewport(0, 0, NW, NH);
  GLuint prog = P.nis;
  glUseProgram(prog);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, P.nisCoef);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, src_.tex);
  glUniform1i(glGetUniformLocation(prog, "u_video"), 0);
  glUniform1i(glGetUniformLocation(prog, "u_coef"), 1);
  glUniform2i(glGetUniformLocation(prog, "u_max"), src_.w - 1, src_.h - 1);
  glUniform2f(glGetUniformLocation(prog, "u_scale"), (float)src_.w / NW, (float)src_.h / NH);
  // NVScalerUpdateConfig (SDR) at sharpness 0.5 (NVIDIA's default).
  const float sharpness = 0.5f;
  float slider = sharpness - 0.5f;
  float maxScale = slider >= 0 ? 1.25f : 1.75f, minScale = slider >= 0 ? 1.25f : 1.f,
        limitScale = slider >= 0 ? 1.25f : 1.f;
  float strengthMin = std::max(0.f, 0.4f + slider * minScale * 1.2f);
  float strengthMax = 1.6f + slider * maxScale * 1.8f;
  float limitMin = std::max(0.1f, 0.14f + slider * limitScale * 0.32f);
  float limitMax = 0.5f + slider * limitScale * 0.6f;
  auto u = [&](const char* n, float v) { glUniform1f(glGetUniformLocation(prog, n), v); };
  u("kDetectRatio", 2.f * 1127.f / 1024.f);
  u("kDetectThres", 64.f / 1024.f);
  u("kMinContrastRatio", 2.f);
  u("kRatioNorm", 1.f / (10.f - 2.f));
  u("kSharpStartY", 0.45f);
  u("kSharpScaleY", 1.f / (0.9f - 0.45f));
  u("kSharpStrengthMin", strengthMin);
  u("kSharpStrengthScale", strengthMax - strengthMin);
  u("kSharpLimitMin", limitMin);
  u("kSharpLimitScale", limitMax - limitMin);
  P.quad(prog, NW, NH, 0, 0, (float)NW, (float)NH);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  const GpuImage* out = &nisOut_;
  if (W != NW || H != NH) {
    lanczos(nisOut_, W, H, lzOut_);
    out = &lzOut_;
  }
  if (c.identity()) {
    fxOut_ = out;
  } else {
    runColor(*out, W, H, c);
  }
}

// Anime4K CNN x2 on src_, then to W × H: straight through at exactly 2×,
// FSR EASU for the rest of the way above 2× (SD sources), a bilinear
// downsample below it. Returns false when the network is unavailable.
bool VideoPipeline::runAnime4K(int W, int H, const ColorAdjust& c) {
  auto& P = GpuPrograms::get();
  const std::vector<GLuint>& progs = P.anime4k();
  if (progs.empty()) return false;
  if (!a4kTex_) a4kTex_ = new Anime4KTextures();
  const int sw = src_.w, sh = src_.h;
  const size_t n = progs.size();
  for (size_t i = 0; i < n; ++i) {
    const Anime4KPass& pass = kAnime4KNet[i];
    bool last = std::strcmp(pass.save, "MAIN") == 0;
    GpuImage& out = last ? a4kOut_ : a4kTex_->get(pass.save);
    int ow = sw * pass.scale, oh = sh * pass.scale;
    out.ensure(ow, oh, last ? GL_RGBA8 : GL_RGBA16F);
    glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);
    if (!last && P.anime4kState == 1 && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::fprintf(stderr, "Anime4K: float framebuffer incomplete\n");
      P.anime4kState = -1;
      for (GLuint q : P.a4k) glDeleteProgram(q);
      P.a4k.clear();
      return false;
    }
    glViewport(0, 0, ow, oh);
    GLuint prog = progs[i];
    glUseProgram(prog);
    for (int b = 0; b < pass.nbind; ++b) {
      std::string name = pass.binds[b];
      const GpuImage& t = name == "MAIN" ? src_ : a4kTex_->get(name);
      glActiveTexture(GL_TEXTURE0 + b);
      glBindTexture(GL_TEXTURE_2D, t.tex);
      glUniform1i(glGetUniformLocation(prog, (name + "_raw").c_str()), b);
      glUniform2f(glGetUniformLocation(prog, (name + "_size").c_str()), (float)t.w, (float)t.h);
    }
    P.quad(prog, ow, oh, 0, 0, (float)ow, (float)oh);
  }
  for (int b = 7; b >= 0; --b) {
    glActiveTexture(GL_TEXTURE0 + b);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (W > a4kOut_.w + 1) {
    // Above 2×: FSR EASU does the remaining step (RCAS off: the network's
    // lines are already crisp), colour adjustments in the final pass.
    easu_.ensure(W, H);
    glBindFramebuffer(GL_FRAMEBUFFER, easu_.fbo);
    glViewport(0, 0, W, H);
    glUseProgram(P.easu);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, a4kOut_.tex);
    glUniform1i(glGetUniformLocation(P.easu, "u_video"), 0);
    float sx = (float)a4kOut_.w / W, sy = (float)a4kOut_.h / H;
    glUniform4f(glGetUniformLocation(P.easu, "u_con0"), sx, sy, 0.5f * sx - 0.5f, 0.5f * sy - 0.5f);
    glUniform2i(glGetUniformLocation(P.easu, "u_max"), a4kOut_.w - 1, a4kOut_.h - 1);
    P.quad(P.easu, W, H, 0, 0, (float)W, (float)H);
    runColor(easu_, W, H, c);
  } else if (W == a4kOut_.w && H == a4kOut_.h && c.identity()) {
    fxOut_ = &a4kOut_;
  } else {
    runColor(a4kOut_, W, H, c);
  }
  return true;
}

Readback::~Readback() { reset(); }

void Readback::reset() {
  for (auto& s : slots_) {
    if (s.fence) glDeleteSync(s.fence);
    if (s.pbo) glDeleteBuffers(1, &s.pbo);
    s = Slot{};
  }
  order_.clear();
  next_ = 0;
}

void Readback::request(const GpuImage& img, double t) {
  Slot& s = slots_[next_];
  if (s.pending) {
    // Ring full: the oldest request is dropped (encoder is behind).
    if (s.fence) glDeleteSync(s.fence);
    s.fence = nullptr;
    s.pending = false;
    for (auto it = order_.begin(); it != order_.end(); ++it)
      if (*it == next_) {
        order_.erase(it);
        break;
      }
  }
  size_t need = (size_t)img.w * img.h * 4;
  GLint prevFbo;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevFbo);
  if (!s.pbo) glGenBuffers(1, &s.pbo);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo);
  if (s.size != need) {
    glBufferData(GL_PIXEL_PACK_BUFFER, need, nullptr, GL_STREAM_READ);
    s.size = need;
  }
  glBindFramebuffer(GL_READ_FRAMEBUFFER, img.fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels(0, 0, img.w, img.h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  s.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, prevFbo);
  s.w = img.w;
  s.h = img.h;
  s.t = t;
  s.pending = true;
  order_.push_back(next_);
  next_ = (next_ + 1) % 4;
}

bool Readback::poll(std::vector<uint8_t>& out, int& w, int& h, double& t, bool wait) {
  if (order_.empty()) return false;
  Slot& s = slots_[order_.front()];
  GLenum r = glClientWaitSync(s.fence, wait ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, wait ? 100000000ull : 0);
  if (r != GL_ALREADY_SIGNALED && r != GL_CONDITION_SATISFIED) return false;
  glDeleteSync(s.fence);
  s.fence = nullptr;
  glBindBuffer(GL_PIXEL_PACK_BUFFER, s.pbo);
  void* p = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, s.size, GL_MAP_READ_BIT);
  bool ok = false;
  if (p) {
    out.resize(s.size);
    std::memcpy(out.data(), p, s.size);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    ok = true;
  }
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  w = s.w;
  h = s.h;
  t = s.t;
  s.pending = false;
  order_.pop_front();
  return ok;
}

void readImage(const GpuImage& img, std::vector<uint8_t>& out) {
  out.resize((size_t)img.w * img.h * 4);
  GLint prevFbo;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevFbo);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, img.fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels(0, 0, img.w, img.h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
  glBindFramebuffer(GL_READ_FRAMEBUFFER, prevFbo);
}

// ---------------------------------------------------------------------------
#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif
#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#endif

GpuTimer::~GpuTimer() { release(); }

void GpuTimer::release() {
  for (int i = 0; i < N; ++i)
    if (q_[i]) glDeleteQueries(1, &q_[i]), q_[i] = 0, pending_[i] = false;
  active_ = -1;
}

void GpuTimer::poll() {
  for (int i = 0; i < N; ++i) {
    if (!pending_[i] || i == active_) continue;
    GLuint avail = 0;
    glGetQueryObjectuiv(q_[i], GL_QUERY_RESULT_AVAILABLE, &avail);
    if (!avail) continue;
    GLuint ns = 0;
    glGetQueryObjectuiv(q_[i], GL_QUERY_RESULT, &ns);
    pending_[i] = false;
    if (!counted_[i]) continue;
    double ms = ns / 1e6;
    avg_ = avg_ < 0 ? ms : avg_ * 0.9 + ms * 0.1;
  }
}

void GpuTimer::begin() {
  if (supported_ < 0) {
    supported_ = 1;
    if (supported_) glGenQueries(N, q_);
  }
  if (!supported_ || active_ >= 0) return;
  poll();
  for (int k = 0; k < N; ++k) {
    int i = (head_ + k) % N;
    if (pending_[i]) continue;
    while (glGetError() != GL_NO_ERROR) {
    }
    glBeginQuery(GL_TIME_ELAPSED, q_[i]);
    if (glGetError() != GL_NO_ERROR) {
      supported_ = 0;  // driver refuses timer queries
      return;
    }
    active_ = i;
    head_ = (i + 1) % N;
    return;
  }
}

void GpuTimer::end(bool count) {
  if (active_ < 0) return;
  glEndQuery(GL_TIME_ELAPSED);
  pending_[active_] = true;
  counted_[active_] = count;
  active_ = -1;
}

// ---------------------------------------------------------------------------
void Backdrop::update(const GpuImage& in) {
  if (!in.tex || in.w <= 0) return;
  GlStateGuard guard;
  auto& P = GpuPrograms::get();
  // Halve (exact 2×2 box average with bilinear taps) down to ≤ 160 wide.
  const GpuImage* src = &in;
  int w = in.w, h = in.h, i = 0;
  glUseProgram(P.blit);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(P.blit, "u_tex"), 0);
  while (w > 160 && i < 4) {
    w = std::max(1, w / 2);
    h = std::max(1, h / 2);
    down[i].ensure(w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, down[i].fbo);
    glViewport(0, 0, w, h);
    glBindTexture(GL_TEXTURE_2D, src->tex);
    P.quad(P.blit, w, h, 0, 0, (float)w, (float)h);
    src = &down[i++];
  }
  // Three rounds of separable blur (1-texel taps: no aliasing grid);
  // saturation lifted on the last pass.
  a.ensure(w, h);
  b.ensure(w, h);
  glUseProgram(P.blur);
  glUniform1i(glGetUniformLocation(P.blur, "u_tex"), 0);
  for (int round = 0; round < 3; ++round) {
    glBindFramebuffer(GL_FRAMEBUFFER, a.fbo);
    glViewport(0, 0, w, h);
    glBindTexture(GL_TEXTURE_2D, round == 0 ? src->tex : b.tex);
    glUniform2f(glGetUniformLocation(P.blur, "u_dir"), 1.f / w, 0.f);
    glUniform1f(glGetUniformLocation(P.blur, "u_sat"), 1.f);
    P.quad(P.blur, w, h, 0, 0, (float)w, (float)h);
    glBindFramebuffer(GL_FRAMEBUFFER, b.fbo);
    glBindTexture(GL_TEXTURE_2D, a.tex);
    glUniform2f(glGetUniformLocation(P.blur, "u_dir"), 0.f, 1.f / h);
    glUniform1f(glGetUniformLocation(P.blur, "u_sat"), round == 2 ? 1.35f : 1.f);
    P.quad(P.blur, w, h, 0, 0, (float)w, (float)h);
  }
  ready = true;
}

void Backdrop::release() {
  for (auto& d : down) d.release();
  a.release();
  b.release();
  ready = false;
}
