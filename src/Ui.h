// Small drawing toolkit on top of Dear ImGui's draw lists: typography
// (letter-spacing, uppercase eyebrows, CJK-aware wrapping, line-height),
// theme colour tokens and the UVC Studio brand gradient.
#pragma once
#include <string>
#include <vector>

#include "imgui.h"

namespace ui {

// ---- colour tokens ----
ImU32 rgba(int r, int g, int b, float a = 1.f);
ImU32 withAlpha(ImU32 c, float a);  // multiplies alpha
ImU32 mix(ImU32 a, ImU32 b, float t);
// Theme tokens; updated by setTheme(). kRed = recording / errors, kGreen =
// live / OK; kBlue and kPurple are brand accents. kInk is the foreground
// base colour for translucent overlays (white on dark, near-black on light).
extern ImU32 kRed, kGreen, kBlue, kPurple, kBg, kPanel, kPanel2, kFg, kMuted, kBorder, kBorderHover, kStage, kPopover,
    kSettingsBg, kInk, kShadow;
ImU32 ink(float alpha);  // kInk with an alpha
// 0 = dark theme, 1 = light theme (fractions blend during the transition).
void setTheme(float light);
float themeLight();

// ---- brand gradient: green → blue → purple → red ----
ImU32 gradient(float t, float alpha = 1.f);
// Recolours vertices [v0, v1) of `dl` along the axis p0 → p1 with the brand
// gradient (t0..t1 of it), keeping each vertex's alpha (× alphaMul).
void shadeGradient(ImDrawList* dl, int v0, int v1, ImVec2 p0, ImVec2 p1, float alphaMul = 1.f, float t0 = 0.f,
                   float t1 = 1.f);
void gradientRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, float alpha = 1.f);
void gradientBorder(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, float thickness, float alpha = 1.f);
// Soft radial glow (centre colour fading to transparent at radius r).
void radialGlow(ImDrawList* dl, ImVec2 c, float r, ImU32 col, int segments = 64);

// ---- fonts ----
// Manrope: display (wordmark, headings, buttons). IBM Plex Sans: body,
// labels and values (its figures are tabular).
enum class Family { Manrope, Plex };
bool loadFonts(const std::string& assetsDir);
ImFont* font(Family f, int weight);

struct TextStyle {
  ImFont* f = nullptr;
  float size = 14.f;     // px (already UI-scaled)
  float tracking = 0.f;  // em
  bool upper = false;
  float lineHeight = 0.f;  // multiplier; 0 → font natural
};
TextStyle ts(Family fam, int weight, float size, float trackingEm = 0.f, bool upper = false, float lh = 0.f);

float scale();  // UI scale factor
void setScale(float s);
inline float px(float v) { return v * scale(); }

std::string upper(const std::string& s);
float textWidth(const TextStyle& s, const std::string& text);
float lineHeightPx(const TextStyle& s);
// Draws one line. `pos` = top-left of the line box.
void drawText(ImDrawList* dl, ImVec2 pos, const TextStyle& s, ImU32 col, const std::string& text);
std::string ellipsize(const TextStyle& s, const std::string& text, float maxW);

struct Line {
  std::string text;
  float width = 0;
};
// Greedy wrap: honours '\n', breaks at spaces and between CJK characters.
std::vector<Line> wrap(const TextStyle& s, const std::string& text, float maxW);
// Draws wrapped text; align: 0 left, 1 centre. Returns height used.
float drawWrapped(ImDrawList* dl, ImVec2 pos, float maxW, const TextStyle& s, ImU32 col, const std::string& text,
                  int align = 0);
float wrappedHeight(const TextStyle& s, const std::string& text, float maxW);

// Text filled with the brand gradient (left → right across its width).
void drawGradientText(ImDrawList* dl, ImVec2 pos, const TextStyle& s, const std::string& text, float alpha = 1.f);
float drawGradientWrapped(ImDrawList* dl, ImVec2 pos, float maxW, const TextStyle& s, const std::string& text,
                          int align = 0);

// Rounded rectangle with a 1px border.
void panel(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 fill, ImU32 border, float rounding, float borderW = 1.f);
// Soft drop shadow approximation.
void shadow(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, float spread, float alpha);

// ---- Liquid Glass ----
// The blurred picture behind the UI (live video, blurred), mapped over the
// rectangle it is drawn at; glass surfaces sample it to look frosted. tex 0
// = none (plain translucency).
void setBackdrop(ImTextureID tex, ImVec2 a, ImVec2 b);
// Controls drawn on top of a glass panel: plain translucency, no second
// backdrop sample (avoids colour blotches inside the panel).
void suspendBackdrop(bool on);
// Multiplies the alpha of vertices [v0, v1) by a ramp a0 → a1 along y0 → y1.
void alphaRamp(ImDrawList* dl, int v0, int v1, float y0, float y1, float a0, float a1);
struct GlassStyle {
  float solid = 0.f;       // 0 airy … 1 nearly opaque (menus with text)
  ImU32 tint = 0;          // optional colour wash (e.g. red for "End")
  float tintAmount = 0.f;
  bool gradientTint = false;  // brand gradient wash (active / primary)
  float shadowAlpha = 0.45f;
  bool lens = true;        // magnify the backdrop slightly (refraction)
};
// Frosted, translucent rounded surface: backdrop, fill, top sheen, bright rim.
void glass(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, const GlassStyle& st = GlassStyle());

}  // namespace ui
