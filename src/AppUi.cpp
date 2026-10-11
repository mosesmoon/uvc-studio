// UVC Studio interface: layout and drawing (design spec §9). Dark / light
// themes, Liquid Glass surfaces and the green → blue → purple → red brand
// gradient.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>

#include "App.h"
#include "I18n.h"
#include "Icons.h"
#include "Ui.h"
#include "gl.h"
#include "imgui_internal.h"
#include "stb_image.h"

using namespace ui;

// ---------------------------------------------------------------------------
// Design tokens (spec §9.1): type scale in px at 100 % (ts() applies the
// interface scale), radii and the disabled opacity.
namespace tk {
TextStyle wordmark() { return ts(Family::Manrope, 800, 17); }
TextStyle heading() { return ts(Family::Manrope, 700, 28); }
TextStyle title() { return ts(Family::Manrope, 700, 17); }  // sheet titles, wordmark size
TextStyle section() { return ts(Family::Manrope, 700, 13, 0.02f); }
TextStyle button() { return ts(Family::Manrope, 700, 14); }
TextStyle buttonSmall() { return ts(Family::Manrope, 600, 13); }
TextStyle body() { return ts(Family::Plex, 500, 14); }
TextStyle bodyPara() { return ts(Family::Plex, 500, 14, 0, false, 1.5f); }
TextStyle label() { return ts(Family::Plex, 500, 12); }
TextStyle value() { return ts(Family::Plex, 600, 12); }
TextStyle chip() { return ts(Family::Plex, 600, 12); }
TextStyle keycap() { return ts(Family::Plex, 600, 11); }
constexpr float kTileR = 14, kPanelR = 18, kFieldR = 10;
constexpr float kDisabled = 0.35f;  // opacity of disabled controls
}  // namespace tk

namespace {

bool inRect(ImVec2 p, ImVec2 a, ImVec2 b) { return p.x >= a.x && p.y >= a.y && p.x < b.x && p.y < b.y; }

// Invisible hit area in the current window.
bool hit(const char* id, ImVec2 a, ImVec2 b, bool enabled, bool* hovered = nullptr, bool* held = nullptr) {
  ImGui::SetCursorScreenPos(a);
  ImVec2 sz(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y));
  bool clicked = ImGui::InvisibleButton(id, sz);
  bool hov = ImGui::IsItemHovered();
  if (hovered) *hovered = hov;
  if (held) *held = ImGui::IsItemActive();
  if (hov) ImGui::SetMouseCursor(enabled ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_NotAllowed);
  return clicked && enabled;
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

ImU32 blend(ImU32 base, ImU32 over) {
  // Composite `over` (with alpha) onto opaque `base`.
  float a = ((over >> IM_COL32_A_SHIFT) & 0xff) / 255.f;
  auto ch = [&](int sh) {
    float b = (float)((base >> sh) & 0xff), o = (float)((over >> sh) & 0xff);
    return (int)std::lround(b + (o - b) * a);
  };
  return IM_COL32(ch(IM_COL32_R_SHIFT), ch(IM_COL32_G_SHIFT), ch(IM_COL32_B_SHIFT), 255);
}

// Draws an icon in the brand gradient.
void gradientIcon(ImDrawList* dl, const char* name, ImVec2 c, float size, float fbScale, float alpha = 1.f) {
  int v0 = dl->VtxBuffer.Size;
  icons::draw(dl, name, c, size, IM_COL32_WHITE, fbScale);
  shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(c.x - size / 2, c.y - size / 2), ImVec2(c.x + size / 2, c.y + size / 2),
                alpha);
}

// Third-party components listed under Settings → About → Open-source licenses.
struct Component {
  const char* name;
  const char* license;
  const char* file;  // assets/licenses/<file>
  const char* note;
};
const std::vector<Component>& components() {
  static const std::vector<Component> c = {
      {"UVC Studio", "MIT", "MIT-uvcstudio.txt",
       "UVC Studio for Linux and Windows is open source. Source: https://github.com/mosesmoon/uvc-studio"},
#if defined(_WIN32)
      {"FFmpeg 7.1", "LGPL-2.1-or-later", "LGPL-2.1.txt",
       "Built without GPL components: recording uses the graphics card's or Windows' own H.264 encoder. "
       "https://ffmpeg.org"},
      {"SDL 2", "Zlib", "Zlib.txt", "https://www.libsdl.org"},
#else
      {"FFmpeg 7.1", "LGPL-2.1-or-later", "LGPL-2.1.txt",
       "Built without GPL components: recording uses the graphics card's hardware H.264 encoder "
       "(VA-API, NVENC or AMF). https://ffmpeg.org"},
      {"SDL 2", "Zlib", "Zlib.txt", "https://www.libsdl.org"},
#endif
      {"Dear ImGui", "MIT", "MIT-imgui.txt", "https://github.com/ocornut/imgui"},
      {"miniaudio", "Public domain / MIT-0", "MIT-0.txt", "https://miniaud.io"},
      {"stb_image, stb_image_write", "Public domain / MIT", "MIT-stb.txt", "https://github.com/nothings/stb"},
      {"NanoSVG", "Zlib", "Zlib-nanosvg.txt", "https://github.com/memononen/nanosvg"},
      {"AMD FidelityFX Super Resolution 1.0 (EASU, RCAS)", "MIT", "MIT-fsr.txt",
       "Ported to OpenGL / OpenGL ES shaders. https://github.com/GPUOpen-Effects/FidelityFX-FSR"},
      {"Anime4K v4 (Upscale CNN x2)", "MIT", "MIT-anime4k.txt",
       "Network weights and shaders converted from the mpv version. https://github.com/bloc97/Anime4K"},
      {"AMD FidelityFX CAS", "MIT", "MIT-cas.txt",
       "Ported to OpenGL / OpenGL ES shaders. https://github.com/GPUOpen-Effects/FidelityFX-CAS"},
      {"NVIDIA Image Scaling SDK 1.0.3", "MIT", "MIT-nis.txt",
       "Scaler ported to OpenGL / OpenGL ES fragment shaders. https://github.com/NVIDIAGameWorks/NVIDIAImageScaling"},
      {"Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC (subset)", "SIL Open Font License 1.1",
       "OFL-1.1.txt",
       "https://github.com/sharanda/manrope · https://github.com/IBM/plex · https://github.com/notofonts/noto-cjk"},
  };
  return c;
}

}  // namespace

// ---------------------------------------------------------------------------
static void applyStyle(float S) {
  ImGuiStyle& st = ImGui::GetStyle();
  st = ImGuiStyle();
  ImGui::StyleColorsDark(&st);
  st.WindowPadding = ImVec2(0, 0);
  st.WindowRounding = 0;
  st.WindowBorderSize = 0;
  st.FramePadding = ImVec2(10 * S, 8 * S);
  st.FrameRounding = 6 * S;
  st.FrameBorderSize = 1;
  st.PopupRounding = 8 * S;
  st.PopupBorderSize = 1;
  st.ItemSpacing = ImVec2(8 * S, 6 * S);
  st.ItemInnerSpacing = ImVec2(6 * S, 4 * S);
  st.ScrollbarSize = 10 * S;
  st.ScrollbarRounding = 8 * S;
  st.GrabRounding = 4 * S;
  st.SelectableTextAlign = ImVec2(0, 0.5f);
  auto c = [](ImU32 u) { return ImGui::ColorConvertU32ToFloat4(u); };
  ImVec4* col = st.Colors;
  col[ImGuiCol_Text] = c(kFg);
  col[ImGuiCol_TextDisabled] = c(kMuted);
  col[ImGuiCol_WindowBg] = c(IM_COL32(0, 0, 0, 0));
  col[ImGuiCol_ChildBg] = c(IM_COL32(0, 0, 0, 0));
  col[ImGuiCol_PopupBg] = c(kSettingsBg);
  col[ImGuiCol_Border] = c(kBorder);
  col[ImGuiCol_FrameBg] = c(kPanel2);
  col[ImGuiCol_FrameBgHovered] = c(kPanel2);
  col[ImGuiCol_FrameBgActive] = c(kPanel2);
  col[ImGuiCol_Header] = c(withAlpha(kBlue, 0.18f));
  col[ImGuiCol_HeaderHovered] = c(ink(0.07f));
  col[ImGuiCol_HeaderActive] = c(withAlpha(kBlue, 0.28f));
  col[ImGuiCol_ScrollbarBg] = c(IM_COL32(0, 0, 0, 0));
  col[ImGuiCol_ScrollbarGrab] = c(ink(0.16f));
  col[ImGuiCol_ScrollbarGrabHovered] = c(ink(0.27f));
  col[ImGuiCol_ScrollbarGrabActive] = c(ink(0.35f));
  col[ImGuiCol_Button] = c(kPanel2);
  col[ImGuiCol_NavCursor] = c(IM_COL32(0, 0, 0, 0));
}

// Drop-down field (rounded, chevron at the right).
struct Opt {
  std::string value;
  std::string label;
};
static bool selectBox(const char* id, const std::vector<Opt>& opts, const std::string& value, std::string& out,
                      float width, bool* opened = nullptr) {
  std::string preview;
  for (auto& o : opts)
    if (o.value == value) preview = o.label;
  if (preview.empty() && !opts.empty()) preview = opts.front().label;
  TextStyle t = tk::body();
  ImGui::PushFont(t.f, t.size);
  ImGui::SetNextItemWidth(width);
  ImVec2 p0 = ImGui::GetCursorScreenPos();
  bool changed = false;
  float S = scale();
  // Leave room for the chevron.
  std::string shown = ellipsize(t, preview, width - 44 * S);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12 * S, 8 * S));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10 * S);
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12 * S);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(ink(0.06f)));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorConvertU32ToFloat4(ink(0.09f)));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(ink(0.09f)));
  // The field's rectangle (once the list is open, the "last item" is the
  // list window, so it is not taken from there).
  ImVec2 p1(p0.x + width, p0.y + ImGui::GetFrameHeight());
  ImDrawList* dl = ImGui::GetWindowDrawList();
  bool open = ImGui::BeginCombo(id, shown.c_str(), ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_HeightLarge);
  ImGui::PopStyleColor(3);
  bool hov = !open && ImGui::IsItemHovered();
  if (hov || open) dl->AddRect(p0 + ImVec2(0.5f, 0.5f), p1 + ImVec2(-0.5f, -0.5f), kBorderHover, 10 * S);
  if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  icons::draw(dl, "chevron", ImVec2(p1.x - 16 * S, (p0.y + p1.y) * 0.5f), 14 * S, kMuted,
              ImGui::GetIO().DisplayFramebufferScale.x);
  if (open) {
    if (opened) *opened = true;
    for (auto& o : opts) {
      bool sel = o.value == value;
      if (ImGui::Selectable((o.label + "##" + o.value).c_str(), sel, 0, ImVec2(0, 26 * S))) {
        out = o.value;
        changed = true;
      }
      if (sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::PopStyleVar(3);
  ImGui::PopFont();
  return changed;
}

// Knob of a range input: white disc with a gradient ring.
static void sliderKnob(ImDrawList* dl, ImVec2 c, float r, float t, float alpha) {
  dl->AddCircleFilled(c, r + 1.5f * scale(), gradient(t, alpha), 28);
  dl->AddCircleFilled(c, r - 1.5f * scale(), withAlpha(IM_COL32(255, 255, 255, 255), alpha), 28);
}

// Horizontal range input (gradient fill).
static bool rangeSlider(const char* id, float& v, float mn, float mx, float step, ImVec2 a, float w, float h,
                        bool enabled = true) {
  float S = scale();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  bool hov, held;
  hit(id, a, ImVec2(a.x + w, a.y + h), enabled, &hov, &held);
  bool changed = false;
  if (held && enabled) {
    float t = std::clamp((ImGui::GetIO().MousePos.x - a.x) / w, 0.f, 1.f);
    float nv = mn + t * (mx - mn);
    nv = std::round(nv / step) * step;
    nv = std::clamp(nv, mn, mx);
    if (std::fabs(nv - v) > 1e-5f) {
      v = nv;
      changed = true;
    }
  }
  float t = (v - mn) / (mx - mn);
  float cy = a.y + h * 0.5f;
  float r = 8 * S;
  float x0 = a.x + r * 0.5f, x1 = a.x + w - r * 0.5f;
  float tx = lerp(x0, x1, t);
  float alpha = enabled ? 1.f : 0.4f;
  dl->AddRectFilled(ImVec2(x0, cy - 2 * S), ImVec2(x1, cy + 2 * S), ink(0.16f * alpha), 2 * S);
  if (tx > x0 + 1) {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(ImVec2(x0, cy - 2 * S), ImVec2(tx, cy + 2 * S), IM_COL32_WHITE, 2 * S);
    shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(x0, cy), ImVec2(x1, cy), alpha);
  }
  sliderKnob(dl, ImVec2(tx, cy), r * (hov || held ? 1.08f : 1.f), t, alpha);
  return changed;
}

static bool vRangeSlider(const char* id, float& v, ImVec2 a, float w, float h, bool enabled) {
  float S = scale();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  bool hov, held;
  hit(id, a, ImVec2(a.x + w, a.y + h), enabled, &hov, &held);
  bool changed = false;
  if (held && enabled) {
    float t = std::clamp((a.y + h - ImGui::GetIO().MousePos.y) / h, 0.f, 1.f);
    float nv = std::round(t * 100.f) / 100.f;
    if (nv != v) v = nv, changed = true;
  }
  if (hov && enabled && ImGui::GetIO().MouseWheel != 0) {
    v = std::clamp(v + ImGui::GetIO().MouseWheel * 0.05f, 0.f, 1.f);
    v = std::round(v * 100.f) / 100.f;
    changed = true;
  }
  float cx = a.x + w * 0.5f;
  float r = 8 * S;
  float y0 = a.y + h - r * 0.5f, y1 = a.y + r * 0.5f;
  float ty = lerp(y0, y1, v);
  float al = enabled ? 1.f : 0.4f;
  dl->AddRectFilled(ImVec2(cx - 2 * S, y1), ImVec2(cx + 2 * S, y0), ink(0.16f * al), 2 * S);
  if (ty < y0 - 1) {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(ImVec2(cx - 2 * S, ty), ImVec2(cx + 2 * S, y0), IM_COL32_WHITE, 2 * S);
    shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(cx, y0), ImVec2(cx, y1), al);
  }
  sliderKnob(dl, ImVec2(cx, ty), r, v, al);
  return changed;
}

// ---------------------------------------------------------------------------
// Theme: dark, light, or automatic by the local time of day (light from 06:00
// to 18:00). Changes cross-fade over ~0.4 s.
void App::updateTheme() {
  float target;
  if (themePref_ == "dark") {
    target = 0.f;
  } else if (themePref_ == "light") {
    target = 1.f;
  } else {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    target = (tm.tm_hour >= 6 && tm.tm_hour < 18) ? 1.f : 0.f;
  }
  if (themeMix_ < 0) {
    themeMix_ = target;  // first frame: no fade
  } else if (themeMix_ != target) {
    float step = ImGui::GetIO().DeltaTime * 2.5f;
    themeMix_ = themeMix_ < target ? std::min(target, themeMix_ + step) : std::max(target, themeMix_ - step);
  }
  float eased = themeMix_ * themeMix_ * (3.f - 2.f * themeMix_);
  setTheme(eased);
}


// ---------------------------------------------------------------------------
// Controls shared by the top bar, dock, popovers, settings and sheets.

static float fbs() {
  float f = ImGui::GetIO().DisplayFramebufferScale.x;
  return f > 0 ? f : 1.f;
}

// Shortcut text ("Ctrl+D", "Shift+U", "F") as separate keys.
static std::vector<std::string> splitKeys(const std::string& s) {
  std::vector<std::string> keys;
  std::string cur;
  for (char c : s) {
    if (c == '+' && !cur.empty()) {
      keys.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  if (!cur.empty()) keys.push_back(cur);
  return keys;
}

// Keycaps drawn left to right from `p` (left edge, vertical centre). Returns
// the width; draws only when `draw`. `onColor` = on a brand-gradient surface.
static float keycaps(ImDrawList* dl, ImVec2 p, const std::vector<std::string>& keys, bool draw, bool onColor = false) {
  float S = scale();
  TextStyle t = tk::keycap();
  const float h = std::round(18 * S), padX = 5 * S, gap = 3 * S;
  float x = p.x;
  for (size_t i = 0; i < keys.size(); ++i) {
    float tw = textWidth(t, keys[i]);
    float w = std::max(h, std::round(tw + 2 * padX));
    if (draw) {
      ImVec2 a(x, std::round(p.y - h / 2)), b(x + w, a.y + h);
      ImU32 fill = onColor ? IM_COL32(255, 255, 255, 46) : ink(0.07f);
      ImU32 edge = onColor ? IM_COL32(255, 255, 255, 90) : ink(0.16f);
      dl->AddRectFilled(a, b, fill, 5 * S);
      dl->AddRect(a, b, edge, 5 * S, 0, 1.f);
      // A slightly heavier bottom edge gives the key its depth.
      dl->AddLine(ImVec2(a.x + 4 * S, b.y - 0.5f), ImVec2(b.x - 4 * S, b.y - 0.5f), edge, 1.f);
      drawText(dl, ImVec2(x + (w - tw) / 2, a.y + (h - lineHeightPx(t)) / 2 - 0.5f * S), t,
               onColor ? IM_COL32_WHITE : kFg, keys[i]);
    }
    x += w + (i + 1 < keys.size() ? gap : 0);
  }
  return x - p.x;
}

// On / off switch, 36 × 20: the track fills with the brand gradient when on
// and the knob slides across.
static bool toggleSwitch(const char* id, bool on, ImVec2 p, bool enabled = true) {
  float S = scale();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float w = std::round(36 * S), h = std::round(20 * S);
  bool hov = false;
  bool clicked = hit(id, p, p + ImVec2(w, h), enabled, &hov);
  static std::map<std::string, float> anim;
  auto it = anim.find(id);
  if (it == anim.end()) it = anim.emplace(id, on ? 1.f : 0.f).first;
  float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.1f);
  it->second += ((on ? 1.f : 0.f) - it->second) * (1.f - std::exp(-dt * 20.f));
  float t = it->second, al = enabled ? 1.f : tk::kDisabled;
  dl->AddRectFilled(p, p + ImVec2(w, h), ink((hov && enabled ? 0.22f : 0.16f) * al), h / 2);
  if (t > 0.01f) {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(p, p + ImVec2(w, h), withAlpha(IM_COL32_WHITE, t), h / 2);
    shadeGradient(dl, v0, dl->VtxBuffer.Size, p, p + ImVec2(w, h), al);
  }
  float r = h / 2 - 2.5f * S;
  ImVec2 c(p.x + h / 2 + t * (w - h), p.y + h / 2);
  dl->AddCircleFilled(c + ImVec2(0, 0.8f * S), r + 0.6f * S, withAlpha(kShadow, 0.22f * al), 24);
  dl->AddCircleFilled(c, r, withAlpha(IM_COL32_WHITE, al), 24);
  return clicked;
}

// Pill button on glass: primary = brand gradient, otherwise neutral glass.
// Icon, label and optional keycaps are centred as a group.
static bool glassButton(ImDrawList* dl, const char* id, ImVec2 a, ImVec2 b, const std::string& label,
                        const char* icon, bool primary, bool enabled, const TextStyle& t,
                        const std::vector<std::string>& keys = {}, bool* hovered = nullptr) {
  float S = scale();
  bool hov = false;
  bool clicked = hit(id, a, b, enabled, &hov);
  if (hovered) *hovered = hov;
  int v0 = dl->VtxBuffer.Size;
  GlassStyle gs;
  gs.lens = false;
  gs.shadowAlpha = primary ? 0.35f : 0.18f;
  if (primary) {
    gs.gradientTint = true;
    gs.tintAmount = hov && enabled ? 0.96f : 0.86f;
  } else {
    gs.solid = 0.25f;
    if (hov && enabled) {
      gs.tint = kInk;
      gs.tintAmount = 0.08f;
    }
  }
  float h = b.y - a.y;
  glass(dl, a, b, h / 2, gs);
  const float iconSz = std::round(16 * S), iconGap = 8 * S, keyGap = 10 * S;
  float kw = keys.empty() ? 0.f : keycaps(dl, ImVec2(0, 0), keys, false);
  float lw = textWidth(t, label);
  float maxLabel = (b.x - a.x) - 24 * S - (icon ? iconSz + iconGap : 0) - (kw > 0 ? kw + keyGap : 0);
  std::string shown = ellipsize(t, label, std::max(20 * S, maxLabel));
  lw = textWidth(t, shown);
  float total = (icon ? iconSz + iconGap : 0) + lw + (kw > 0 ? keyGap + kw : 0);
  float x = std::round((a.x + b.x) / 2 - total / 2), cy = (a.y + b.y) / 2;
  ImU32 fg = primary ? IM_COL32_WHITE : kFg;
  if (icon) {
    icons::draw(dl, icon, ImVec2(x + iconSz / 2, cy), iconSz, fg, fbs());
    x += iconSz + iconGap;
  }
  drawText(dl, ImVec2(x, std::round(cy - lineHeightPx(t) / 2)), t, fg, shown);
  x += lw;
  if (kw > 0) keycaps(dl, ImVec2(x + keyGap, cy), keys, true, primary);
  if (!enabled) alphaRamp(dl, v0, dl->VtxBuffer.Size, 0, 1, tk::kDisabled, tk::kDisabled);
  return clicked;
}

// Status dot of the ready-screen checklist: 1 found (solid green with a soft
// halo), 0 missing (hollow ring), 2 optional and absent (faint ring).
static void statusDot(ImDrawList* dl, ImVec2 c, int state) {
  float S = scale();
  float r = 4 * S;
  if (state == 1) {
    dl->AddCircleFilled(c, r + 3 * S, withAlpha(kGreen, 0.22f), 24);
    dl->AddCircleFilled(c, r, kGreen, 24);
  } else {
    dl->AddCircle(c, r - 0.75f * S, state == 0 ? kMuted : ink(0.28f), 24, 1.5f * S);
  }
}

// Corner picker: a miniature 16:9 picture with the overlay in the chosen
// corner; clicking one of the four quarters picks that corner. Returns the
// picked corner or -1.
static int cornerPicker(const char* id, int corner, ImVec2 p, float w, float h) {
  float S = scale();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(p, p + ImVec2(w, h), ink(0.06f), 8 * S);
  dl->AddRect(p, p + ImVec2(w, h), ink(0.10f), 8 * S, 0, 1.f);
  int picked = -1;
  float bw = std::round(w * 0.36f), bh = std::round(bw * 9.f / 16.f), m = std::round(4 * S);
  for (int c = 0; c < 4; ++c) {
    bool left = c == App::CornerTopLeft || c == App::CornerBottomLeft;
    bool top = c == App::CornerTopLeft || c == App::CornerTopRight;
    ImVec2 qa(p.x + (left ? 0 : w / 2), p.y + (top ? 0 : h / 2)), qb(qa.x + w / 2, qa.y + h / 2);
    bool hv = false;
    std::string hid = std::string(id) + "#" + std::to_string(c);
    if (hit(hid.c_str(), qa, qb, true, &hv)) picked = c;
    ImVec2 ba(left ? p.x + m : p.x + w - m - bw, top ? p.y + m : p.y + h - m - bh);
    if (c == corner)
      gradientRect(dl, ba, ba + ImVec2(bw, bh), 3 * S);
    else
      dl->AddRectFilled(ba, ba + ImVec2(bw, bh), ink(hv ? 0.34f : 0.14f), 3 * S);
  }
  return picked;
}

static const char* cornerKey(int c) {
  static const char* k[4] = {"overlay.cornerTopLeft", "overlay.cornerTopRight", "overlay.cornerBottomLeft",
                             "overlay.cornerBottomRight"};
  return k[std::clamp(c, 0, 3)];
}

// Picture control value: "+12", "0", "−8".
static std::string signedValue(int v) {
  if (v > 0) return "+" + std::to_string(v);
  if (v < 0) return "\xE2\x88\x92" + std::to_string(-v);
  return "0";
}

// Popover placed centred above an anchor, kept inside the window.
static ImVec2 popoverPos(ImVec2 anchor, float W, float H) {
  float S = scale();
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImVec2 pos(std::round(anchor.x - W / 2), std::round(anchor.y - 10 * S - H));
  pos.x = std::max(vp->Pos.x + 8 * S, std::min(pos.x, vp->Pos.x + vp->Size.x - 8 * S - W));
  return pos;
}

static const char* kUpscalerShort[4] = {"FSR", "Anime4K", "Lanczos", "NIS"};

// ---------------------------------------------------------------------------
// Ready screen backdrop: three soft glows in the brand colours, drifting
// slowly behind the glass card, top bar and dock.
static void ambientGlows(ImDrawList* dl, ImVec2 a, ImVec2 b) {
  float W = b.x - a.x, H = b.y - a.y;
  double t = nowSeconds();
  float al = 0.15f - 0.05f * themeLight();
  struct Glow {
    float x, y, r, phase, hue;
  };
  const Glow glows[3] = {{0.24f, 0.32f, 0.42f, 0.0f, 0.05f}, {0.78f, 0.30f, 0.38f, 2.1f, 0.45f},
                         {0.56f, 0.86f, 0.46f, 4.2f, 0.92f}};
  for (const Glow& g : glows) {
    ImVec2 c(a.x + W * (g.x + 0.045f * (float)std::sin(t * 0.05 + g.phase)),
             a.y + H * (g.y + 0.045f * (float)std::cos(t * 0.04 + g.phase)));
    radialGlow(dl, c, std::max(W, H) * g.r, gradient(g.hue, al), 72);
  }
}

// ---------------------------------------------------------------------------
void App::frame() {
  ImGuiIO& io = ImGui::GetIO();
  fbScale_ = io.DisplayFramebufferScale.x > 0 ? io.DisplayFramebufferScale.x : 1.f;
  updateTheme();
  // Interface size: automatic (grows with the display's resolution / DPI)
  // or a fixed percentage from Settings, times the Ctrl +/- zoom.
  int display = SDL_GetWindowDisplayIndex(win_);
  autoScale_ = platform::displayScale(display);
  float baseScale = uiScalePct_ > 0 ? uiScalePct_ / 100.f : autoScale_;
  float S = std::round(baseScale * uiZoom_ * 20.f) / 20.f;
  if (S != appliedScale_ || themeLight() != appliedTheme_) {
    setScale(S);
    applyStyle(S);
    appliedScale_ = S;
    appliedTheme_ = themeLight();
  }

  tick();

  anyComboOpen_ = false;
  tooltip_.clear();
  tooltipKey_.clear();
  tooltipBelow_ = false;

  // ---- root window ----
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGui::Begin("##root", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoNavFocus);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImVec2 a = vp->Pos, b = ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y);
  // Immersive (VR) mode: while connected the picture is on the giant screen,
  // so the panel's stage stays transparent.
  bool xrLive = xrMode_ && connected_;
  if (!xrLive) dl->AddRectFilled(a, b, kBg);
  // Liquid Glass: while connected, a blurred, dimmed copy of the picture
  // fills the window and the glass surfaces sample it.
  bool ambient = connected_ && !xrMode_ && backdrop_.ready;
  if (ambient) {
    ImTextureID bt = (ImTextureID)(intptr_t)backdrop_.image().tex;
    ui::setBackdrop(bt, a, b);
    if (!fullscreen_) {
      dl->AddImage(ImTextureRef(bt), a, b, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
      dl->AddRectFilled(a, b, withAlpha(kBg, 0.66f + 0.12f * themeLight()));
    }
  } else {
    ui::setBackdrop(0, a, b);
    if (!connected_) ambientGlows(dl, a, b);
  }
  // Layout: top bar 56, dock area 88 (60 shelf + 14 margin above and below),
  // 16 px side gutters around the stage.
  const float topH = std::round(56 * S), dockH = std::round(88 * S), side = std::round(16 * S);
  stageRefMin_ = ImVec2(a.x + side, a.y + topH);
  stageRefMax_ = ImVec2(b.x - side, b.y - dockH);
  if (fullscreen_) {
    stageMin_ = a;
    stageMax_ = b;
    drawStage(dl, a, b);
  } else {
    stageMin_ = stageRefMin_;
    stageMax_ = stageRefMax_;
    drawStage(dl, stageMin_, stageMax_);
    drawDock(dl, ImVec2(a.x, b.y - dockH), b);
    drawTopbar(dl, a, ImVec2(b.x, a.y + topH));
  }
  ImGui::End();

  drawPopovers();
  if (aboutOpen_) drawAbout();
  ImDrawList* fg = ImGui::GetForegroundDrawList();
  if (shortcutsOpen_) {
    tooltip_.clear();
    drawShortcutSheet(fg);
  }
  drawToasts(fg);
  drawTooltip(fg);
}

// ---------------------------------------------------------------------------
// Top bar: wordmark (left), connection chip (centre) with the error line
// under it, language pill and Connect / Disconnect (right).
void App::drawTopbar(ImDrawList* dl, ImVec2 a, ImVec2 b) {
  float S = scale();
  const std::string& L = lang_;
  ImGuiIO& io = ImGui::GetIO();
  const float pad = std::round(20 * S), cy = std::round((a.y + b.y) / 2), h = std::round(34 * S);

  TextStyle wm = tk::wordmark();
  const std::string brand = "UVC Studio";
  float wmW = textWidth(wm, brand);
  drawGradientText(dl, ImVec2(a.x + pad, std::round(cy - lineHeightPx(wm) / 2)), wm, brand);

  // Connect / Disconnect.
  TextStyle bt = tk::button();
  std::string cl = i18n::tr(L, connected_ ? "session.disconnect" : "session.connect");
  float cw = std::round(14 * S + 16 * S + 8 * S + textWidth(bt, cl) + 16 * S);
  ImVec2 c1(b.x - pad, cy + h / 2), c0(c1.x - cw, cy - h / 2);
  bool canPress = connected_ || canConnect();
  bool chv = false;
  if (glassButton(dl, "##connect", c0, c1, cl, connected_ ? "unplug" : "plug", !connected_, canPress, bt, {}, &chv)) {
    if (connected_)
      disconnect();
    else
      connect();
  }
  if (chv) {
    tooltip_ = canPress ? cl : i18n::tr(L, "session.waiting");
    tooltipKey_ = canPress ? (connected_ ? "Ctrl+D" : "Enter") : "";
    tooltipAnchor_ = ImVec2((c0.x + c1.x) / 2, c1.y + 8 * S);
    tooltipBelow_ = true;
  }

  // Language pill.
  TextStyle lt = tk::chip();
  std::string ln = i18n::tr(L, "lang.name");
  const float icon = std::round(16 * S);
  float lw = std::round(12 * S + icon + 7 * S + textWidth(lt, ln) + 6 * S + 12 * S + 10 * S);
  ImVec2 l1(c0.x - 10 * S, cy + h / 2), l0(l1.x - lw, cy - h / 2);
  bool lhv = false;
  if (hit("##langpill", l0, l1, true, &lhv)) langOpen_ = !langOpen_;
  {
    GlassStyle gs;
    gs.solid = 0.25f;
    gs.lens = false;
    gs.shadowAlpha = 0.18f;
    if (lhv || langOpen_) {
      gs.tint = kInk;
      gs.tintAmount = 0.08f;
    }
    glass(dl, l0, l1, h / 2, gs);
    icons::draw(dl, "globe", ImVec2(l0.x + 12 * S + icon / 2, cy), icon, kFg, fbScale_);
    drawText(dl, ImVec2(l0.x + 12 * S + icon + 7 * S, std::round(cy - lineHeightPx(lt) / 2)), lt, kFg, ln);
    icons::draw(dl, "chevron", ImVec2(l1.x - 10 * S - 6 * S, cy), 12 * S, kMuted, fbScale_);
  }
  langBtnMin_ = l0;
  langBtnMax_ = l1;
  if (lhv && !langOpen_) {
    tooltip_ = i18n::tr(L, "lang.title");
    tooltipAnchor_ = ImVec2((l0.x + l1.x) / 2, l1.y + 8 * S);
    tooltipBelow_ = true;
  }
  // A click anywhere else closes the language menu.
  if (langOpen_ && ImGui::IsMouseClicked(0) && !inRect(io.MousePos, l0, l1) &&
      !inRect(io.MousePos, langMenuMin_, langMenuMax_))
    langOpen_ = false;

  // Connection chip, centred in the window (shifted when space is short).
  float left = a.x + pad + wmW + 24 * S, right = l0.x - 24 * S;
  float maxW = std::max(60 * S, right - left);
  float w = drawStatus(dl, 0, cy, maxW, false);
  float mid = (a.x + b.x) / 2;
  float x = std::round(std::clamp(mid - w / 2, left, std::max(left, right - w)));
  drawStatus(dl, x, cy, maxW, true);
  drawErrorLine(dl, x + w / 2, cy + std::round(15 * S) + 6 * S);
}

// Connection chip: hollow dot + "Not connected"; green dot + "Device ·
// 1920 × 1080 · 60 fps" (+ " › 4K FSR", + " · 3D"); while recording a
// pulsing red dot + "REC 03:12". Returns the width; draws when `draw`.
float App::drawStatus(ImDrawList* dl, float x, float cy, float maxW, bool draw) {
  float S = scale();
  const std::string& L = lang_;
  TextStyle t = tk::chip();
  const float h = std::round(30 * S), padX = 12 * S, dot = 8 * S, gap = 8 * S;
  std::string text;
  if (recording_) {
    int secs = std::max(0, (int)(nowSeconds() - recStart_));
    char buf[48];
    if (secs >= 3600)
      std::snprintf(buf, sizeof buf, "%s %d:%02d:%02d", i18n::tr(L, "status.rec"), secs / 3600, secs / 60 % 60,
                    secs % 60);
    else
      std::snprintf(buf, sizeof buf, "%s %02d:%02d", i18n::tr(L, "status.rec"), secs / 60, secs % 60);
    text = buf;
  } else if (connected_) {
    int w = streamActual_.width ? streamActual_.width : modeW_;
    int hgt = streamActual_.width ? streamActual_.height : modeH_;
    int fps = streamActual_.fps > 0 ? (int)std::lround(streamActual_.fps) : modeFps_;
    std::string rest = " \xC2\xB7 " + resolutionText(w, hgt) + " \xC2\xB7 " + fpsLabel(fps);
    // U+203A (single right-pointing angle quote): in Manrope / IBM Plex Sans.
    if (upscale_) rest += std::string(" \xE2\x80\xBA 4K ") + kUpscalerShort[std::clamp(upscaleMode_, 0, 3)];
    if (depth3d_) rest += " \xC2\xB7 3D";
    std::string dev = currentVideoLabel();
    if (dev.empty()) dev = i18n::tr(L, "status.live");
    float avail = maxW - 2 * padX - dot - gap - textWidth(t, rest);
    text = ellipsize(t, dev, std::max(48 * S, avail)) + rest;
  } else {
    text = i18n::tr(L, "status.notConnected");
  }
  text = ellipsize(t, text, std::max(20 * S, maxW - 2 * padX - dot - gap));
  float w = std::round(padX + dot + gap + textWidth(t, text) + padX);
  if (!draw) return w;
  ImVec2 a(x, cy - h / 2), b(x + w, cy + h / 2);
  GlassStyle gs;
  gs.solid = 0.3f;
  gs.lens = false;
  gs.shadowAlpha = 0.18f;
  if (recording_) {
    gs.tint = kRed;
    gs.tintAmount = 0.1f;
  }
  glass(dl, a, b, h / 2, gs);
  ImVec2 dc(a.x + padX + dot / 2, cy);
  if (recording_) {
    float ph = 0.5f + 0.5f * (float)std::cos(nowSeconds() * 6.2831853 / 0.8);
    dl->AddCircleFilled(dc, dot / 2 + 3 * S * ph, withAlpha(kRed, 0.25f * ph), 24);
    dl->AddCircleFilled(dc, dot / 2, withAlpha(kRed, 0.55f + 0.45f * ph), 24);
  } else if (connected_) {
    dl->AddCircleFilled(dc, dot / 2 + 3 * S, withAlpha(kGreen, 0.22f), 24);
    dl->AddCircleFilled(dc, dot / 2, kGreen, 24);
  } else {
    dl->AddCircle(dc, dot / 2 - 0.75f * S, kMuted, 24, 1.5f * S);
  }
  drawText(dl, ImVec2(dc.x + dot / 2 + gap, std::round(cy - lineHeightPx(t) / 2)), t, kFg, text);
  return w;
}

// Error line under the chip: red text on a small glass pill, fading out at
// the end of its 6 s.
void App::drawErrorLine(ImDrawList* dl, float centerX, float top) {
  std::string e = errorText();
  if (e.empty()) return;
  float S = scale();
  TextStyle t = tk::label();
  ImGuiViewport* vp = ImGui::GetMainViewport();
  const float h = std::round(24 * S), padX = 12 * S, dot = 6 * S, gap = 7 * S;
  float maxW = std::min(vp->Size.x * 0.6f, 760 * S);
  std::string s = ellipsize(t, e, maxW - 2 * padX - dot - gap);
  float w = std::round(padX + dot + gap + textWidth(t, s) + padX);
  float x = std::clamp(centerX - w / 2, vp->Pos.x + 8 * S, vp->Pos.x + vp->Size.x - 8 * S - w);
  ImVec2 a(std::round(x), std::round(top)), b(a.x + w, a.y + h);
  float fade = std::clamp((float)((kErrorSeconds - (nowSeconds() - errAt_)) / 0.4), 0.f, 1.f);
  int v0 = dl->VtxBuffer.Size;
  GlassStyle gs;
  gs.solid = 0.6f;
  gs.lens = false;
  gs.shadowAlpha = 0.25f;
  gs.tint = kRed;
  gs.tintAmount = 0.1f;
  glass(dl, a, b, h / 2, gs);
  dl->AddCircleFilled(ImVec2(a.x + padX + dot / 2, a.y + h / 2), dot / 2, kRed, 16);
  drawText(dl, ImVec2(a.x + padX + dot + gap, std::round(a.y + (h - lineHeightPx(t)) / 2)), t, kRed, s);
  if (fade < 1.f) alphaRamp(dl, v0, dl->VtxBuffer.Size, 0, 1, fade, fade);
}

// ---------------------------------------------------------------------------
// Stage: the ready screen while disconnected; the picture (letterboxed, with
// rounded corners unless fullscreen) and the webcam overlay while connected.
void App::drawStage(ImDrawList* dl, ImVec2 a, ImVec2 b) {
  float S = scale();
  ImGuiIO& io = ImGui::GetIO();
  picMin_ = picMax_ = ImVec2(0, 0);
  if (!connected_) {
    if (fullscreen_) toggleFullscreen();
    drawIdle(dl, a, b);
    return;
  }
  if (xrMode_) return;  // the picture is on the giant VR screen
  bool stereo = stereoActive() && stereoReady_ && stereoImg_.tex;
  const GpuImage& img = stereo ? stereoImg_ : pipe_.view();
  float rad = fullscreen_ ? 0.f : tk::kTileR * S;
  if (fullscreen_) dl->AddRectFilled(a, b, kStage);
  if (!pipe_.hasImage() || img.w <= 0 || img.h <= 0) {
    dl->AddRectFilled(a, b, kStage, rad);  // waiting for the first frame
    return;
  }
  float aspect = (float)img.w / img.h;
  float W = b.x - a.x, H = b.y - a.y;
  float w = W, h = W / aspect;
  if (h > H) h = H, w = H * aspect;
  ImVec2 p0(std::round(a.x + (W - w) / 2), std::round(a.y + (H - h) / 2));
  ImVec2 p1(p0.x + std::round(w), p0.y + std::round(h));
  picMin_ = p0;
  picMax_ = p1;
  ImVec2 uv0(0, 0), uv1(1, 1);
  if (mirror_ && !stereo) std::swap(uv0.x, uv1.x);
  if (!fullscreen_) shadow(dl, p0, p1, rad, 30 * S, 0.5f);
  dl->AddImageRounded(ImTextureRef((ImTextureID)(intptr_t)img.tex), p0, p1, uv0, uv1, IM_COL32_WHITE, rad);
  if (!fullscreen_) dl->AddRect(p0, p1, kBorder, rad, 0, 1.f);

  bool overOverlay = false;
  if (!stereo) {
    float r[4];
    if (overlayRectNorm(r)) {
      ImVec2 o0(p0.x + r[0] * (p1.x - p0.x), p0.y + r[1] * (p1.y - p0.y));
      ImVec2 o1(p0.x + r[2] * (p1.x - p0.x), p0.y + r[3] * (p1.y - p0.y));
      overOverlay = inRect(io.MousePos, o0, o1);
    }
    drawOverlay(dl);
  }
  bool rootHovered = ImGui::IsWindowHovered();
  // Double-click (double-tap on Quest) the picture: fullscreen on / off.
  if (rootHovered && inRect(io.MousePos, p0, p1) && !overOverlay && !overlayDragging_ &&
      ImGui::IsMouseDoubleClicked(0))
    toggleFullscreen();
  // A click on the stage outside the Settings panel closes it.
  if (settingsOpen_ && rootHovered && !anyComboOpen_ && ImGui::IsMouseClicked(0) && inRect(io.MousePos, a, b))
    settingsOpen_ = false;
}

// Webcam overlay on the picture: drag it anywhere and it settles into the
// nearest corner (150 ms ease-out); the wheel steps its size; hovering shows
// a glass toolbar (Mirror, Size, Remove) along its inner edge.
void App::drawOverlay(ImDrawList* dl) {
  float S = scale();
  ImGuiIO& io = ImGui::GetIO();
  double now = nowSeconds();
  float pw = picMax_.x - picMin_.x, ph = picMax_.y - picMin_.y;
  float r[4];
  if (pw <= 0 || ph <= 0 || !overlayRectNorm(r)) {
    if (overlayDragging_ || overlaySnapping_) setOverlayDisplayRect(nullptr);
    overlayDragging_ = overlaySnapping_ = false;
    return;
  }
  auto toScreen = [&](const float* q, ImVec2& o0, ImVec2& o1) {
    o0 = ImVec2(std::round(picMin_.x + q[0] * pw), std::round(picMin_.y + q[1] * ph));
    o1 = ImVec2(std::round(picMin_.x + q[2] * pw), std::round(picMin_.y + q[3] * ph));
  };
  ImVec2 o0, o1;
  toScreen(r, o0, o1);
  bool rootHovered = ImGui::IsWindowHovered();
  bool over = rootHovered && inRect(io.MousePos, o0, o1);
  if (over || overlayDragging_) overlayChromeUntil_ = now + 0.3;
  bool chrome = now < overlayChromeUntil_ && !overlayDragging_ && !overlaySnapping_;

  // Toolbar hit areas first, so its buttons take the clicks over the drag
  // area underneath; drawn after the camera picture.
  bool topHalf = (r[1] + r[3]) * 0.5f < 0.5f;
  const float bs = std::round(28 * S), inner = 2 * S, edge = 3 * S, barH = bs + 2 * inner;
  const float barW = 3 * bs + 2 * inner + 2 * edge;
  bool showBar = chrome && (o1.x - o0.x) > barW + 16 * S && (o1.y - o0.y) > barH + 16 * S;
  ImVec2 t0(std::round((o0.x + o1.x) / 2 - barW / 2), topHalf ? o1.y - 8 * S - barH : o0.y + 8 * S);
  ImVec2 t1(t0.x + barW, t0.y + barH);
  struct Btn {
    const char* id;
    const char* icon;
    const char* key;
    bool active;
    bool hov;
  } btns[3] = {{"##ovmirror", "mirror", "overlay.mirror", overlayMirror_, false},
               {"##ovsize", "size", "overlay.size", false, false},
               {"##ovremove", "close", "overlay.remove", false, false}};
  auto btnRect = [&](int i, ImVec2& b0, ImVec2& b1) {
    b0 = ImVec2(t0.x + edge + i * (bs + inner), t0.y + inner);
    b1 = b0 + ImVec2(bs, bs);
  };
  if (showBar) {
    for (int i = 0; i < 3; ++i) {
      ImVec2 b0, b1;
      btnRect(i, b0, b1);
      if (hit(btns[i].id, b0, b1, true, &btns[i].hov)) {
        if (i == 0) setOverlayMirror(!overlayMirror_);
        if (i == 1) cycleOverlaySize();
        if (i == 2) {
          setOverlayOn(false);
          return;
        }
      }
      if (btns[i].hov) {
        tooltip_ = i18n::tr(lang_, btns[i].key);
        if (i == 1) tooltip_ += std::string(" \xC2\xB7 ") + "SML"[std::clamp(overlaySize_, 0, 2)];
        tooltipAnchor_ = ImVec2((b0.x + b1.x) / 2, topHalf ? t1.y + 8 * S : t0.y - 8 * S);
        tooltipBelow_ = topHalf;
      }
    }
  }

  // Drag area.
  bool dragHover = false, held = false;
  hit("##overlaydrag", o0, o1, true, &dragHover, &held);
  if (dragHover) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  if (held && !overlayDragging_ && ImGui::IsMouseDragging(0, 4 * S)) {
    overlayDragging_ = true;
    overlaySnapping_ = false;
    overlayGrab_ = io.MouseClickedPos[0] - o0;
  }
  if (overlayDragging_) {
    if (ImGui::IsMouseDown(0)) {
      float w = r[2] - r[0], h = r[3] - r[1];
      float nx = std::clamp((io.MousePos.x - overlayGrab_.x - picMin_.x) / pw, 0.f, 1.f - w);
      float ny = std::clamp((io.MousePos.y - overlayGrab_.y - picMin_.y) / ph, 0.f, 1.f - h);
      float q[4] = {nx, ny, nx + w, ny + h};
      setOverlayDisplayRect(q);
    } else {
      // Released: settle into the nearest corner.
      std::copy(r, r + 4, overlaySnapFrom_);
      setOverlayCorner(nearestCorner((r[0] + r[2]) / 2, (r[1] + r[3]) / 2));
      overlayDragging_ = false;
      overlaySnapping_ = true;
      overlaySnapStart_ = now;
      setOverlayDisplayRect(overlaySnapFrom_);
    }
  }
  if (overlaySnapping_) {
    float t = std::clamp((float)((now - overlaySnapStart_) / 0.15), 0.f, 1.f);
    if (t >= 1.f) {
      overlaySnapping_ = false;
      setOverlayDisplayRect(nullptr);
    } else {
      float e = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
      float to[4], q[4];
      overlayRectFor(overlayCorner_, overlaySize_, pw / ph, to);
      for (int i = 0; i < 4; ++i) q[i] = lerp(overlaySnapFrom_[i], to[i], e);
      setOverlayDisplayRect(q);
    }
  }
  // Mouse wheel over the overlay: one size step per notch.
  static float wheel = 0;
  if (over && !overlayDragging_ && io.MouseWheel != 0) {
    wheel += io.MouseWheel;
    if (std::fabs(wheel) >= 1.f) {
      stepOverlaySize(wheel > 0 ? 1 : -1);
      wheel = 0;
    }
  }

  // Camera picture, centre-cropped to the box, rounded corners.
  if (!overlayRectNorm(r)) return;
  toScreen(r, o0, o1);
  const GpuImage& cam = overlayPipe_.view();
  float boxAspect = (o1.x - o0.x) / std::max(1.f, o1.y - o0.y);
  float camAspect = (float)cam.w / std::max(1, cam.h);
  ImVec2 uv0(0, 0), uv1(1, 1);
  if (camAspect > boxAspect) {
    uv0.x = 0.5f * (1.f - boxAspect / camAspect);
    uv1.x = 1.f - uv0.x;
  } else if (camAspect < boxAspect) {
    uv0.y = 0.5f * (1.f - camAspect / boxAspect);
    uv1.y = 1.f - uv0.y;
  }
  if (overlayMirror_) std::swap(uv0.x, uv1.x);
  const float rad = 10 * S;
  shadow(dl, o0, o1, rad, 20 * S, 0.5f);
  dl->AddImageRounded(ImTextureRef((ImTextureID)(intptr_t)cam.tex), o0, o1, uv0, uv1, IM_COL32_WHITE, rad);
  if (over || overlayDragging_)
    gradientBorder(dl, o0, o1, rad, 1.5f * S);
  else
    dl->AddRect(o0, o1, IM_COL32(255, 255, 255, 46), rad, 0, 1.f);

  if (showBar) {
    GlassStyle gs;
    gs.solid = 0.5f;
    gs.lens = false;
    gs.shadowAlpha = 0.35f;
    glass(dl, t0, t1, barH / 2, gs);
    for (int i = 0; i < 3; ++i) {
      ImVec2 b0, b1;
      btnRect(i, b0, b1);
      if (btns[i].active)
        gradientRect(dl, b0, b1, bs / 2, 0.9f);
      else if (btns[i].hov)
        dl->AddRectFilled(b0, b1, ink(0.12f), bs / 2);
      icons::draw(dl, btns[i].icon, (b0 + b1) * 0.5f, 16 * S, btns[i].active ? IM_COL32_WHITE : kFg, fbScale_);
    }
  }
}

// ---------------------------------------------------------------------------
// The app icon (assets/images/uvc-studio-256.png) as a texture, loaded once
// and halved on the CPU so it stays crisp at its 48 px size.
static GLuint loadAppIcon() {
  std::string path = platform::joinPath(platform::joinPath(platform::assetsDir(), "images"), "uvc-studio-256.png");
  std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
  if (!f) return 0;
  std::vector<unsigned char> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  int w = 0, h = 0, n = 0;
  unsigned char* px = stbi_load_from_memory(file.data(), (int)file.size(), &w, &h, &n, 4);
  if (!px) return 0;
  int hw = std::max(1, w / 2), hh = std::max(1, h / 2);
  std::vector<unsigned char> half((size_t)hw * hh * 4);
  for (int y = 0; y < hh; ++y)
    for (int x = 0; x < hw; ++x)
      for (int c = 0; c < 4; ++c) {
        int sx = std::min(w - 1, x * 2), sy = std::min(h - 1, y * 2);
        int s = px[((size_t)sy * w + sx) * 4 + c] + px[((size_t)sy * w + std::min(w - 1, sx + 1)) * 4 + c] +
                px[((size_t)std::min(h - 1, sy + 1) * w + sx) * 4 + c] +
                px[((size_t)std::min(h - 1, sy + 1) * w + std::min(w - 1, sx + 1)) * 4 + c];
        half[((size_t)y * hw + x) * 4 + c] = (unsigned char)(s / 4);
      }
  stbi_image_free(px);
  GLuint tex = 0;
  GLint prev = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, hw, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, half.data());
  glBindTexture(GL_TEXTURE_2D, prev);
  return tex;
}

// Ready screen (disconnected): a centred glass card with the app icon, the
// title, a live checklist of the devices UVC Studio found, the Connect button
// and a hint line.
void App::drawIdle(ImDrawList* dl, ImVec2 a, ImVec2 b) {
  float S = scale();
  const std::string& L = lang_;
  if (!appIconTried_) {
    appIconTried_ = true;
    appIconTex_ = loadAppIcon();
  }
  const float W = std::round(std::min(560 * S, b.x - a.x - 32 * S));
  const float pad = std::round(32 * S), cw = W - 2 * pad;
  const float iconSz = std::round(48 * S), rowH = std::round(48 * S), btnH = std::round(46 * S);
  TextStyle h1 = tk::heading(), sub = tk::bodyPara(), body = tk::body(), val = tk::value();
  TextStyle hint = ts(Family::Plex, 500, 12, 0, false, 1.45f);
  std::string title = i18n::tr(L, "ready.title"), subT = i18n::tr(L, "ready.sub"), hintT = i18n::tr(L, "ready.hint");
  TextStyle h1w = h1;
  h1w.lineHeight = 1.25f;
  float titleH = wrappedHeight(h1w, title, cw), subH = wrappedHeight(sub, subT, cw),
        hintH = wrappedHeight(hint, hintT, cw);
  float H = pad + iconSz + 18 * S + titleH + 8 * S + subH + 24 * S + 3 * rowH + 24 * S + btnH + 16 * S + hintH + 26 * S;
  H = std::round(H);
  ImVec2 p0(std::round(a.x + (b.x - a.x - W) / 2), std::round(std::max(a.y + 8 * S, a.y + (b.y - a.y - H) / 2)));
  ImVec2 p1(p0.x + W, p0.y + H);
  {
    GlassStyle gs;
    gs.solid = 0.6f;
    gs.shadowAlpha = 0.55f;
    glass(dl, p0, p1, tk::kPanelR * S, gs);
  }
  float x = p0.x + pad, y = p0.y + pad, cx = (p0.x + p1.x) / 2;
  if (appIconTex_)
    dl->AddImageRounded(ImTextureRef((ImTextureID)(intptr_t)appIconTex_), ImVec2(cx - iconSz / 2, y),
                        ImVec2(cx + iconSz / 2, y + iconSz), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 11 * S);
  else
    gradientIcon(dl, "play", ImVec2(cx, y + iconSz / 2), iconSz * 0.8f, fbScale_);
  y += iconSz + 18 * S;
  y += drawWrapped(dl, ImVec2(x, y), cw, h1w, kFg, title, 1);
  y += 8 * S;
  y += drawWrapped(dl, ImVec2(x, y), cw, sub, kMuted, subT, 1);
  y += 24 * S;

  // Checklist (updates live as devices come and go).
  struct Row {
    const char* label;
    std::string value;
    int state;
  };
  std::string audio = currentAudioLabel();
  const VideoDeviceInfo* cam = overlayCam_ != selVideo_ ? findVideo(overlayCam_) : nullptr;
  Row rows[3] = {
      {"ready.captureCard", canConnect() ? currentVideoLabel() : i18n::tr(L, "ready.captureMissing"),
       canConnect() ? 1 : 0},
      {"ready.gameAudio", audio.empty() ? i18n::tr(L, "ready.audioMissing") : audio, audio.empty() ? 0 : 1},
      {"ready.webcam", cam ? cam->name : i18n::tr(L, "ready.none"), cam ? 1 : 2},
  };
  ImVec2 l0(x, std::round(y)), l1(x + cw, std::round(y + 3 * rowH));
  dl->AddRectFilled(l0, l1, ink(0.035f), tk::kTileR * S);
  dl->AddRect(l0, l1, kBorder, tk::kTileR * S, 0, 1.f);
  for (int i = 0; i < 3; ++i) {
    float ry = l0.y + i * rowH, rc = ry + rowH / 2;
    if (i) dl->AddLine(ImVec2(l0.x + 16 * S, ry), ImVec2(l1.x - 16 * S, ry), kBorder, 1.f);
    statusDot(dl, ImVec2(l0.x + 22 * S, rc), rows[i].state);
    std::string lab = i18n::tr(L, rows[i].label);
    float labX = l0.x + 40 * S;
    drawText(dl, ImVec2(labX, std::round(rc - lineHeightPx(body) / 2)), body, kFg, lab);
    float vx0 = labX + textWidth(body, lab) + 20 * S, vx1 = l1.x - 18 * S;
    std::string v = ellipsize(val, rows[i].value, std::max(0.f, vx1 - vx0));
    float vw = textWidth(val, v);
    drawText(dl, ImVec2(vx1 - vw, std::round(rc - lineHeightPx(val) / 2)), val,
             rows[i].state == 1 ? withAlpha(kFg, 0.86f) : kMuted, v);
  }
  y = l1.y + 24 * S;

  // Connect, or a calm waiting state while no capture card is present.
  ImVec2 b0(x, std::round(y)), b1(x + cw, std::round(y + btnH));
  if (canConnect()) {
    if (glassButton(dl, "##readyconnect", b0, b1, i18n::tr(L, "session.connect"), "plug", true, true, tk::button(),
                    {"Enter"}))
      connect();
  } else {
    GlassStyle gs;
    gs.solid = 0.2f;
    gs.lens = false;
    gs.shadowAlpha = 0.12f;
    glass(dl, b0, b1, btnH / 2, gs);
    TextStyle bt = tk::button();
    std::string wt = i18n::tr(L, "session.waiting");
    float tw = textWidth(bt, wt), dotR = 4 * S, total = dotR * 2 + 10 * S + tw;
    float sx = std::round(cx - total / 2), cy = (b0.y + b1.y) / 2;
    float ph = 0.5f + 0.5f * (float)std::sin(nowSeconds() * 3.0);
    dl->AddCircleFilled(ImVec2(sx + dotR, cy), dotR + 3 * S * ph, gradient(0.4f, 0.25f * ph), 24);
    dl->AddCircleFilled(ImVec2(sx + dotR, cy), dotR, gradient(0.4f, 0.6f + 0.4f * ph), 24);
    drawText(dl, ImVec2(sx + 2 * dotR + 10 * S, std::round(cy - lineHeightPx(bt) / 2)), bt, kMuted, wt);
  }
  y = b1.y + 16 * S;
  drawWrapped(dl, ImVec2(x, y), cw, hint, kMuted, hintT, 1);
}
// ---------------------------------------------------------------------------
// Dock tiles on the glass shelf. Hovering magnifies only the tile under the
// pointer; a click makes the tile bounce for 1.5 s. Active toggles are filled
// with the brand gradient and get a dot underneath.
void App::drawTiles(ImDrawList* dl, std::vector<DockItem>& items, float x0, float baseBottom, float tile, float gap) {
  float S = scale();
  ImGuiIO& io = ImGui::GetIO();
  double now = nowSeconds();
  float dt = std::clamp(io.DeltaTime, 0.001f, 0.1f);
  // Base layout (hit areas never move, so hovering is stable).
  float x = x0;
  for (auto& it : items) {
    if (it.sepBefore) x += gap + 1 * S + gap;
    it.baseMin = ImVec2(x, baseBottom - tile);
    it.baseMax = ImVec2(x + tile, baseBottom);
    x += tile + gap;
  }
  // Hover and click area: the tile plus half the gap on each side and 8 px
  // above and below, so the top of a magnified tile still takes the click
  // (hover popovers end above this area).
  auto hitMin = [&](const DockItem& it) { return it.baseMin - ImVec2(gap / 2, 8 * S); };
  auto hitMax = [&](const DockItem& it) { return it.baseMax + ImVec2(gap / 2, 8 * S); };
  int hovered = -1;
  if (ImGui::IsWindowHovered())
    for (size_t i = 0; i < items.size(); ++i)
      if (inRect(io.MousePos, hitMin(items[i]), hitMax(items[i]))) hovered = (int)i;
  for (size_t i = 0; i < items.size(); ++i) {
    DockItem& it = items[i];
    // Separator line before a group.
    if (it.sepBefore) {
      float sx = it.baseMin.x - gap - 0.5f * S;
      dl->AddRectFilled(ImVec2(sx, baseBottom - tile * 0.78f), ImVec2(sx + 1 * S, baseBottom - tile * 0.22f), ink(0.18f));
    }
    bool hov = false;
    bool clicked = hit(it.id, hitMin(it), hitMax(it), !it.disabled, &hov);
    if (it.hoverOut) *it.hoverOut = (int)i == hovered;
    // Magnification, eased: only the tile under the pointer grows.
    float target = ((int)i == hovered && !it.disabled) ? 1.22f : 1.f;
    float& sc = tileScale_[it.id];
    if (sc <= 0) sc = 1.f;
    sc += (target - sc) * (1.f - std::exp(-dt * 16.f));
    // Bounce after a click: three hops, decaying, 1.5 s in total.
    float lift = 0;
    auto bi = tileBounce_.find(it.id);
    if (bi != tileBounce_.end()) {
      double t = now - bi->second;
      if (t >= 1.5) {
        tileBounce_.erase(bi);
      } else {
        float hop = (float)std::fabs(std::sin(t / 0.5 * 3.14159265));
        lift = 18 * S * hop * (float)std::exp(-1.7 * t);
      }
    }
    float size = tile * sc;
    float cx = (it.baseMin.x + it.baseMax.x) / 2;
    ImVec2 p0(cx - size / 2, baseBottom - size - lift), p1(cx + size / 2, baseBottom - lift);
    int v0 = dl->VtxBuffer.Size;
    GlassStyle gs;
    gs.shadowAlpha = 0.3f;
    gs.lens = false;
    if (it.active) {
      if (it.redActive) {
        gs.tint = kRed;
        gs.tintAmount = 0.75f;
      } else {
        gs.gradientTint = true;
        gs.tintAmount = 0.8f;
      }
    } else {
      gs.solid = 0.15f;
      if (hov && !it.disabled) {
        gs.tint = kInk;
        gs.tintAmount = 0.07f;
      }
    }
    float r = tk::kTileR * S * sc;
    glass(dl, p0, p1, r, gs);
    ImU32 ic = it.active ? IM_COL32(255, 255, 255, 255) : kFg;
    icons::draw(dl, it.icon, ImVec2(cx, (p0.y + p1.y) / 2), size * 0.5f, ic, fbScale_);
    if (it.disabled) alphaRamp(dl, v0, dl->VtxBuffer.Size, 0, 1, tk::kDisabled, tk::kDisabled);
    // Active dot under the tile.
    if (it.active) dl->AddCircleFilled(ImVec2(cx, baseBottom + 6 * S), 2.2f * S, it.redActive ? kRed : withAlpha(kFg, 0.85f));
    if (it.anchor) *it.anchor = ImVec2(cx - 18 * S, baseBottom - tile - 12 * S);
    if (hov && !it.disabled) {
      tooltip_ = it.label;
      tooltipKey_ = it.shortcut ? it.shortcut : "";
      tooltipAnchor_ = ImVec2(cx, p0.y - 8 * S);
      tooltipBelow_ = false;
    }
    if (clicked) {
      tooltip_.clear();
      tileBounce_[it.id] = now;
      if (it.onClick) it.onClick();
    }
  }
}

float App::tilesWidth(const std::vector<DockItem>& items, float tile, float gap) const {
  float w = 0;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i) w += gap;
    if (items[i].sepBefore) w += gap + 1 * scale() + gap;
    w += tile;
  }
  return w;
}

// Dock (bottom centre): Capture | Picture | Sources | Settings (+ the VR
// switch on Quest), with the version at the bottom left and the fullscreen
// button at the bottom right.
void App::drawDock(ImDrawList* dl, ImVec2 a, ImVec2 b) {
  float S = scale();
  const std::string& L = lang_;
  const float tile = std::round(44 * S), gap = std::round(8 * S), shelfPad = std::round(8 * S),
              margin = std::round(14 * S);
  std::vector<DockItem> items;
  auto add = [&](const char* id, const char* icon, const char* key, const char* shortcut, bool active, bool disabled,
                 std::function<void()> fn) -> DockItem& {
    DockItem it;
    it.id = id;
    it.icon = icon;
    it.label = i18n::tr(L, key);
    it.shortcut = shortcut;
    it.active = active;
    it.disabled = disabled;
    it.onClick = std::move(fn);
    items.push_back(std::move(it));
    return items.back();
  };
  // 1. Capture.
  add("##dsnap", "snapshot", "dock.snapshot", "S", false, !connected_, [this] { takeSnapshot(); });
  {
    DockItem& rec = add("##drec", "record", recording_ ? "dock.stop" : "dock.record", "R", recording_, !connected_,
                        [this] { toggleRecording(); });
    rec.redActive = true;
  }
  add("##dmic", "mic", "dock.mic", "M", micRec_, false, [this] { setMicRecording(!micRec_); });
  // 2. Picture.
  {
    DockItem& up = add("##dup", "sparkles", "upscale.title", "U", upscale_, false, [this] { toggleUpscale(); });
    up.sepBefore = true;
    up.anchor = &upscaleAnchor_;
    up.hoverOut = &upscaleBtnHover_;
  }
  {
    DockItem& tv = add("##dretro", "tv", "retro.title", "C", retroStyle_ != RetroOff, false, [this] { toggleRetro(); });
    tv.anchor = &retroAnchor_;
    tv.hoverOut = &retroBtnHover_;
  }
  {
    DockItem& d = add("##ddepth", "depth", "depth.title", "D", depth3d_, false, [this] { toggleDepth(); });
    d.anchor = &depthAnchor_;
    d.hoverOut = &depthBtnHover_;
  }
  add("##dmirror", "mirror", "picture.mirror", "X", mirror_, false, [this] { toggleMirror(); });
  // 3. Sources.
  {
    DockItem& o = add("##doverlay", "webcam", "overlay.title", "W", overlayOn_, false, [this] { toggleOverlay(); });
    o.sepBefore = true;
    o.anchor = &overlayAnchor_;
    o.hoverOut = &overlayBtnHover_;
  }
  {
    DockItem& l = add("##dlisten", "audio", "listen.title", "L", listen_, false, [this] { setListen(!listen_); });
    l.anchor = &listenAnchor_;
    l.hoverOut = &listenBtnHover_;
  }
  // 4. Settings (and the 2D ⇄ VR switch on Quest).
  {
    DockItem& s = add("##dsettings", "settings", "settings.title", "O", settingsOpen_, false, [this] {
      settingsOpen_ = !settingsOpen_;
      langOpen_ = false;
    });
    s.sepBefore = true;
    s.anchor = &settingsAnchor_;
  }

  float tw = tilesWidth(items, tile, gap);
  float shelfH = tile + 2 * shelfPad;
  ImVec2 s0(std::round((a.x + b.x) / 2 - tw / 2 - shelfPad), b.y - margin - shelfH);
  ImVec2 s1(s0.x + tw + 2 * shelfPad, b.y - margin);
  {
    GlassStyle gs;
    gs.solid = 0.2f;
    gs.shadowAlpha = 0.45f;
    glass(dl, s0, s1, tk::kPanelR * S + 2 * S, gs);
  }
  drawTiles(dl, items, s0.x + shelfPad, s1.y - shelfPad, tile, gap);
  dockTileTop_ = s1.y - shelfPad - tile;
  float cy = std::round((s0.y + s1.y) / 2);

  // Version, bottom left (hidden when the window is too narrow for it).
  TextStyle vt = tk::label();
  std::string ver = "UVC Studio " + platform::versionLabel();
  if (s0.x - a.x - 20 * S > textWidth(vt, ver) + 16 * S)
    drawText(dl, ImVec2(a.x + 20 * S, std::round(cy - lineHeightPx(vt) / 2)), vt, kMuted, ver);

  // Fullscreen, bottom right.
  const float fb = std::round(38 * S);
  ImVec2 f1(b.x - 20 * S, cy + fb / 2), f0(f1.x - fb, cy - fb / 2);
  bool can = connected_ || fullscreen_;
  bool hv = false;
  if (hit("##fullscreen", f0, f1, can, &hv)) toggleFullscreen();
  int v0 = dl->VtxBuffer.Size;
  GlassStyle gs;
  gs.solid = 0.2f;
  gs.lens = false;
  gs.shadowAlpha = 0.25f;
  if (hv && can) {
    gs.tint = kInk;
    gs.tintAmount = 0.08f;
  }
  glass(dl, f0, f1, tk::kTileR * S * fb / tile, gs);
  icons::draw(dl, fullscreen_ ? "exit-fullscreen" : "fullscreen", (f0 + f1) * 0.5f, 18 * S, kFg, fbScale_);
  if (!can) alphaRamp(dl, v0, dl->VtxBuffer.Size, 0, 1, tk::kDisabled, tk::kDisabled);
  if (hv && can) {
    tooltip_ = i18n::tr(L, fullscreen_ ? "view.exitFullscreen" : "view.fullscreen");
    tooltipKey_ = "F";
    tooltipAnchor_ = ImVec2((f0.x + f1.x) / 2, f0.y - 8 * S);
    tooltipBelow_ = false;
  }
}
// Popovers are separate windows (submitted after the root) so they layer
// above the stage and dock.
void App::drawPopovers() {
  if (fullscreen_) return;
  float S = scale();
  if (settingsOpen_) {
    drawSettingsPopover(settingsAnchor_);
  } else {
    cardPress_.clear();
    if (!cardDrag_.empty()) cancelCardDrag();
  }
  if (langOpen_) drawLangMenu();
  else langMenuMin_ = langMenuMax_ = ImVec2(0, 0);
  auto hoverPopover = [&](bool btnHover, double& until, bool& popHovered, auto draw) {
    double t = nowSeconds();
    // Stays a moment after the pointer leaves (at least a few frames, so a
    // slow frame never closes it on the way from the tile to the card).
    if (btnHover || popHovered) until = t + std::max(0.12, 2.5 * (double)ImGui::GetIO().DeltaTime);
    if (t < until) {
      bool hv = false;
      draw(hv);
      popHovered = hv;
      tooltip_.clear();  // the popover replaces the button's tooltip
    } else {
      popHovered = false;
    }
  };
  {
    static bool popHovered = false;
    hoverPopover(listenBtnHover_, listenHoverUntil_, popHovered,
                 [&](bool& hv) { drawListenPopover(ImVec2(listenAnchor_.x + 18 * S, listenAnchor_.y), hv); });
  }
  {
    static bool popHovered = false;
    hoverPopover(upscaleBtnHover_, upscaleHoverUntil_, popHovered,
                 [&](bool& hv) { drawUpscalePopover(ImVec2(upscaleAnchor_.x + 18 * S, upscaleAnchor_.y), hv); });
  }
  {
    static bool popHovered = false;
    hoverPopover(retroBtnHover_, retroHoverUntil_, popHovered,
                 [&](bool& hv) { drawRetroPopover(ImVec2(retroAnchor_.x + 18 * S, retroAnchor_.y), hv); });
  }
  {
    static bool popHovered = false;
    hoverPopover(depthBtnHover_, depthHoverUntil_, popHovered,
                 [&](bool& hv) { drawDepthPopover(ImVec2(depthAnchor_.x + 18 * S, depthAnchor_.y), hv); });
  }
  {
    static bool popHovered = false;
    hoverPopover(overlayBtnHover_, overlayHoverUntil_, popHovered,
                 [&](bool& hv) { drawOverlayPopover(ImVec2(overlayAnchor_.x + 18 * S, overlayAnchor_.y), hv); });
  }
}

// ---------------------------------------------------------------------------
// Popover / menu surface: Liquid Glass, solid enough for text.
static void popoverBg(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImU32 bg, float r, float solid = 0.72f) {
  (void)bg;
  ImVec2 vmin = ImGui::GetMainViewport()->Pos;
  ImVec2 vmax = vmin + ImGui::GetMainViewport()->Size;
  dl->PushClipRect(vmin, vmax, false);
  GlassStyle gs;
  gs.solid = solid;
  gs.shadowAlpha = 0.6f;
  glass(dl, p0, p1, std::max(r * 1.75f, 14 * scale()), gs);
  dl->PopClipRect();
}

// Segmented control: a translucent track with a glass thumb in
// the brand gradient that slides to the chosen option.
static bool segmented(const char* id, const std::vector<Opt>& opts, std::string& value, ImVec2 p, float w, float h) {
  float S = scale();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  int n = (int)opts.size();
  if (n == 0) return false;
  int sel = 0;
  for (int i = 0; i < n; ++i)
    if (opts[i].value == value) sel = i;
  float segW = w / n;
  static std::map<std::string, float> anim;
  auto it = anim.find(id);
  if (it == anim.end()) it = anim.emplace(id, (float)sel).first;
  float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.1f);
  it->second += (sel - it->second) * (1.f - std::exp(-dt * 18.f));
  if (std::fabs(it->second - sel) < 0.002f) it->second = (float)sel;
  float pos = it->second;
  dl->AddRectFilled(p, p + ImVec2(w, h), ink(0.07f), h / 2);
  dl->AddRect(p, p + ImVec2(w, h), ink(0.07f), h / 2, 0, 1.f);
  float inset = 2.5f * S;
  float tr = (h - 2 * inset) / 2;
  bool changed = false;
  TextStyle t = ts(Family::Plex, 600, 12);
  // Hover highlights under the thumb.
  std::vector<bool> hov(n, false);
  for (int i = 0; i < n; ++i) {
    ImVec2 a(p.x + i * segW, p.y), b(a.x + segW, p.y + h);
    bool hv = false;
    std::string hid = std::string(id) + "#" + std::to_string(i);
    if (hit(hid.c_str(), a, b, true, &hv) && i != sel) {
      value = opts[i].value;
      changed = true;
    }
    hov[i] = hv;
    if (hv && i != sel) dl->AddRectFilled(a + ImVec2(inset, inset), b - ImVec2(inset, inset), ink(0.06f), tr);
  }
  ImVec2 t0(p.x + pos * segW + inset, p.y + inset), t1(t0.x + segW - 2 * inset, p.y + h - inset);
  GlassStyle gs;
  gs.gradientTint = true;
  gs.tintAmount = 0.85f;
  gs.shadowAlpha = 0.22f;
  gs.lens = false;
  glass(dl, t0, t1, tr, gs);
  for (int i = 0; i < n; ++i) {
    ImVec2 a(p.x + i * segW, p.y), b(a.x + segW, p.y + h);
    std::string s = ellipsize(t, opts[i].label, segW - 12 * S);
    float tw = textWidth(t, s);
    float prox = 1.f - std::min(1.f, std::fabs(pos - i));
    ImU32 col = mix(hov[i] ? kFg : kMuted, IM_COL32_WHITE, prox);
    drawText(dl, ImVec2((a.x + b.x) / 2 - tw / 2, p.y + (h - lineHeightPx(t)) / 2), t, col, s);
  }
  return changed;
}

// Dashed outline of a rounded rectangle (white; callers recolour it with
// shadeGradient or draw it in `col`).
static void dashedRoundRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float thick, float dash, float gapLen,
                            ImU32 col) {
  dl->PathClear();
  dl->PathRect(a, b, r);
  std::vector<ImVec2> pts(dl->_Path.begin(), dl->_Path.end());
  dl->PathClear();
  if (pts.size() < 2) return;
  pts.push_back(pts.front());
  // Stretch the pattern so the dashes close evenly around the corner.
  float per = 0;
  for (size_t i = 0; i + 1 < pts.size(); ++i) {
    ImVec2 d = pts[i + 1] - pts[i];
    per += std::sqrt(d.x * d.x + d.y * d.y);
  }
  float unit = dash + gapLen, k = per / std::max(1.f, std::round(per / unit)) / unit;
  dash *= k;
  gapLen *= k;
  std::vector<ImVec2> seg;
  bool on = true;
  float left = dash;
  for (size_t i = 0; i + 1 < pts.size(); ++i) {
    ImVec2 p = pts[i], q = pts[i + 1], d = q - p;
    float L = std::sqrt(d.x * d.x + d.y * d.y), t = 0;
    while (L - t > 1e-3f) {
      float step = std::min(left, L - t);
      if (on) {
        if (seg.empty()) seg.push_back(p + d * (t / L));
        seg.push_back(p + d * ((t + step) / L));
      }
      t += step;
      left -= step;
      if (left <= 1e-3f) {
        if (on && seg.size() >= 2) dl->AddPolyline(seg.data(), (int)seg.size(), col, 0, thick);
        seg.clear();
        on = !on;
        left = on ? dash : gapLen;
      }
    }
  }
  if (on && seg.size() >= 2) dl->AddPolyline(seg.data(), (int)seg.size(), col, 0, thick);
}

// Settings cards (spec §9.6) and their ids in ui.settingsLayout /
// ui.settingsUnlocked.
enum SettingsCard { SecSource, SecSound, SecDepth, SecPicture, SecOverlay, SecDisplay, SecVr, SecGeneral, SecAbout, SecCount };
static const char* kCardIds[SecCount] = {"source", "sound", "depth", "picture", "overlay",
                                         "display", "vr", "general", "about"};
static int cardSec(const std::string& id) {
  for (int i = 0; i < SecCount; ++i)
    if (id == kCardIds[i]) return i;
  return -1;
}
static std::vector<std::string> splitList(const std::string& s, char sep) {
  std::vector<std::string> out;
  size_t a = 0;
  while (a <= s.size()) {
    size_t b = s.find(sep, a);
    if (b == std::string::npos) b = s.size();
    std::string t = s.substr(a, b - a);
    size_t l = t.find_first_not_of(" \t"), r = t.find_last_not_of(" \t");
    out.push_back(l == std::string::npos ? "" : t.substr(l, r - l + 1));
    a = b + 1;
  }
  return out;
}

bool App::customCardLayout() const {
  for (auto& c : cardCols_)
    if (!c.empty()) return true;
  return false;
}

void App::loadCardLayout() {
  for (auto& c : cardCols_) c.clear();
  cardsUnlocked_.clear();
  std::set<std::string> seen;
  std::string saved = settings_.get("ui.settingsLayout");
  if (!saved.empty()) {
    auto cols = splitList(saved, '|');
    for (size_t c = 0; c < cols.size() && c < 3; ++c)
      for (auto& id : splitList(cols[c], ','))
        if (cardSec(id) >= 0 && seen.insert(id).second) cardCols_[c].push_back(id);
  }
  for (auto& id : splitList(settings_.get("ui.settingsUnlocked"), ','))
    if (cardSec(id) >= 0) cardsUnlocked_.insert(id);
  cardsFolded_.clear();
  for (auto& id : splitList(settings_.get("ui.settingsFolded"), ','))
    if (cardSec(id) >= 0) cardsFolded_.insert(id);
}

void App::saveCardLayout() {
  std::string s;
  if (customCardLayout())
    for (int c = 0; c < 3; ++c) {
      if (c) s += '|';
      for (size_t i = 0; i < cardCols_[c].size(); ++i) s += (i ? "," : "") + cardCols_[c][i];
    }
  settings_.set("ui.settingsLayout", s);
  std::string u;
  for (int i = 0; i < SecCount; ++i)
    if (cardsUnlocked_.count(kCardIds[i])) u += (u.empty() ? "" : ",") + std::string(kCardIds[i]);
  settings_.set("ui.settingsUnlocked", u);
  std::string f;
  for (int i = 0; i < SecCount; ++i)
    if (cardsFolded_.count(kCardIds[i])) f += (f.empty() ? "" : ",") + std::string(kCardIds[i]);
  settings_.set("ui.settingsFolded", f);
}

void App::setCardFolded(const std::string& id, bool folded) {
  if (folded == (cardsFolded_.count(id) > 0)) return;
  if (folded)
    cardsFolded_.insert(id);
  else
    cardsFolded_.erase(id);
  // Reversing mid-animation continues from where it is.
  double now = nowSeconds(), t0 = now;
  auto it = cardFoldT0_.find(id);
  if (it != cardFoldT0_.end() && now - it->second < 0.22) t0 = now - (0.22 - (now - it->second));
  cardFoldT0_[id] = t0;
  saveCardLayout();
}

void App::resetCardLayout() {
  cancelCardDrag();
  for (auto& c : cardCols_) c.clear();
  cardsUnlocked_.clear();
  saveCardLayout();
  cardAnimUntil_ = nowSeconds() + 0.3;  // cards glide to the automatic layout
}

void App::setCardUnlocked(const std::string& id, bool unlocked) {
  if (unlocked)
    cardsUnlocked_.insert(id);
  else
    cardsUnlocked_.erase(id);
  saveCardLayout();
}

void App::moveCard(const std::string& id, int col, int visIdx, const std::vector<std::vector<std::string>>& visible) {
  for (auto& c : cardCols_) c.erase(std::remove(c.begin(), c.end(), id), c.end());
  std::vector<std::string>& L = cardCols_[col];
  const std::vector<std::string>& vis = visible[col];
  // Hidden cards (VR outside immersive mode) keep their place in the list.
  size_t pos = L.size();
  auto at = [&](const std::string& v) { return (size_t)(std::find(L.begin(), L.end(), v) - L.begin()); };
  if (visIdx < (int)vis.size())
    pos = std::min(L.size(), at(vis[visIdx]));
  else if (!vis.empty())
    pos = std::min(L.size(), at(vis.back()) + 1);
  L.insert(L.begin() + pos, id);
}

void App::cancelCardDrag() {
  if (!cardDrag_.empty()) {
    // The card flies back from where it was dropped.
    CardAnim& a = cardAnim_[cardDrag_];
    a.cur = a.from = cardGhostRel_;
    a.to = ImVec2(-1e6f, -1e6f);
    a.t0 = nowSeconds();
    cardAnimUntil_ = a.t0 + 0.3;
  }
  cardDrag_.clear();
  cardPress_.clear();
}

// Settings: one large Liquid Glass panel between the top bar and the dock.
// The sections are cards laid out in up to three balanced columns so every option
// is visible and adjustable without scrolling (a scrollbar appears only
// when the window is too small for all of them). Each card has a padlock:
// unlocked cards can be dragged by their header to another column / place,
// and the arrangement is kept (ui.settingsLayout).
void App::drawSettingsPopover(ImVec2 anchor) {
  (void)anchor;
  float S = scale();
  ImGuiViewport* vp = ImGui::GetMainViewport();
  const std::string& L = lang_;
  float top = vp->Pos.y + std::round(56 * S) + 4 * S;
  float bottom = vp->Pos.y + vp->Size.y - std::round(86 * S);  // above the dock shelf
  if (bottom - top < 180 * S) bottom = top + 180 * S;
  float W = std::min(vp->Size.x - 32 * S, 1280 * S);
  ImVec2 pos(vp->Pos.x + (vp->Size.x - W) / 2, top), size(W, bottom - top);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("##settings", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse);
  ImDrawList* wdl = ImGui::GetWindowDrawList();
  popoverBg(wdl, pos, pos + size, kSettingsBg, 15 * S, 0.93f);
  settingsRectMin_ = pos;
  settingsRectMax_ = pos + size;
  ui::suspendBackdrop(true);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

  // Header: title + close (+ "Reset layout", drawn after the cards).
  const float pad = 18 * S;
  ImVec2 resetR1;
  float resetTop = 0;
  {
    TextStyle ht = ts(Family::Manrope, 700, 17);
    float hh = 30 * S, hy = pos.y + pad - 2 * S;
    gradientIcon(wdl, "settings", ImVec2(pos.x + pad + 10 * S, hy + hh / 2), 20 * S, fbScale_);
    drawText(wdl, ImVec2(pos.x + pad + 28 * S, hy + (hh - lineHeightPx(ht)) / 2), ht, kFg, i18n::tr(L, "settings.title"));
    ImVec2 c1(pos.x + W - pad + 4 * S, hy + hh), c0(c1.x - hh, hy);
    bool chv = false;
    if (hit("##setclose", c0, c1, true, &chv)) settingsOpen_ = false;
    GlassStyle cg;
    cg.solid = 0.2f;
    cg.shadowAlpha = 0.2f;
    cg.lens = false;
    if (chv) {
      cg.tint = kInk;
      cg.tintAmount = 0.1f;
    }
    glass(wdl, c0, c1, hh / 2, cg);
    icons::draw(wdl, "close", (c0 + c1) * 0.5f, 14 * S, kFg, fbScale_);
    resetR1 = ImVec2(c0.x - 10 * S, c1.y);
    resetTop = c0.y;
  }
  float gy0 = pos.y + pad + 30 * S + 10 * S;
  ImVec2 g0(pos.x + pad, gy0), g1(pos.x + W - pad, pos.y + size.y - pad + 4 * S);
  ImGui::SetCursorScreenPos(g0);
  ImGui::BeginChild("##settingsGrid", g1 - g0, 0, ImGuiWindowFlags_NoBackground);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImVec2 origin = ImGui::GetCursorScreenPos();  // includes the scroll offset
  float gw = ImGui::GetContentRegionAvail().x;
  float availH = g1.y - g0.y;

  // ---- sections ----
  // Automatic card order (spec §9.6): Source, Sound, Picture, Webcam overlay,
  // Display, VR (Quest immersive mode), General, 3D depth, About.
  static float lastH[SecCount] = {330, 330, 170, 560, 420, 220, 330, 220, 230};  // unscaled px; refined every frame
  // Heights with the card expanded: the automatic layout balances columns by
  // these, so collapsing a card never moves cards to another column.
  static float fullH[SecCount] = {330, 330, 170, 560, 420, 220, 330, 220, 230};
  std::vector<int> secs = {SecSource, SecSound, SecPicture, SecOverlay, SecDisplay};
  if (xrMode_) secs.push_back(SecVr);
  for (int s : {SecGeneral, SecDepth, SecAbout}) secs.push_back(s);
  bool shown[SecCount] = {};
  for (int s : secs) shown[s] = true;
  const float gap = 12 * S, cpad = 16 * S, minColW = 250 * S;
  int n = (int)secs.size();
  int fitK = std::clamp((int)((gw + gap) / (minColW + gap)), 1, 4);
  auto hOf = [&](int sec) { return lastH[sec] * S; };
  std::vector<float> hs(n);
  for (int i = 0; i < n; ++i) hs[i] = fullH[secs[i]] * S;
  auto colH = [&](int i, int j) {
    float s = 0;
    for (int t = i; t < j; ++t) s += hs[t];
    return s + gap * (j - i - 1);
  };
  // Balanced split of the sections (in order) into k columns.
  std::function<float(int, int, std::vector<int>&)> best = [&](int i, int k, std::vector<int>& out) -> float {
    if (k == 1 || n - i <= 1) {
      out.assign(1, n);
      return colH(i, n);
    }
    float bv = 1e9f;
    std::vector<int> bo;
    for (int j = i + 1; j <= n - (k - 1); ++j) {
      // Automatic layout: the 3D depth card stays directly above the About
      // card (same column).
      if (j < n && secs[j] == SecAbout && j > 0 && secs[j - 1] == SecDepth) continue;
      std::vector<int> o;
      float v = std::max(colH(i, j), best(j, k - 1, o));
      if (v < bv) {
        bv = v;
        bo.assign(1, j);
        bo.insert(bo.end(), o.begin(), o.end());
      }
    }
    out = bo;
    return bv;
  };
  // Three columns at most; a too-small window scrolls instead of adding one.
  // The user's arrangement needs all three: a narrower window shows the
  // automatic layout and leaves the saved arrangement untouched.
  const bool arrange = fitK >= 3 && n >= 3;
  ImGuiIO& io = ImGui::GetIO();
  const double now = nowSeconds();
  // Cards the saved arrangement doesn't know yet (VR in immersive mode, cards
  // added by a newer version): VR goes right after Display, others to the
  // bottom of the shortest column.
  if (arrange && customCardLayout()) {
    bool changed = false;
    for (int s : secs) {
      const std::string id = kCardIds[s];
      bool known = false;
      for (auto& c : cardCols_) known |= std::find(c.begin(), c.end(), id) != c.end();
      if (known) continue;
      int bc = -1;
      size_t bpos = 0;
      if (s == SecVr)
        for (int c = 0; c < 3 && bc < 0; ++c) {
          auto it = std::find(cardCols_[c].begin(), cardCols_[c].end(), "display");
          if (it != cardCols_[c].end()) bc = c, bpos = (size_t)(it - cardCols_[c].begin()) + 1;
        }
      if (bc < 0) {
        float bh = 1e9f;
        for (int c = 0; c < 3; ++c) {
          float h = 0;
          for (auto& cid : cardCols_[c])
            if (cardSec(cid) >= 0 && shown[cardSec(cid)]) h += hOf(cardSec(cid)) + gap;
          if (h < bh) bh = h, bc = c;
        }
        bpos = cardCols_[bc].size();
      }
      cardCols_[bc].insert(cardCols_[bc].begin() + bpos, id);
      changed = true;
    }
    if (changed) saveCardLayout();
  }
  // Visible cards per column: the saved arrangement, or the balanced split.
  std::vector<std::vector<int>> cols;
  auto buildCols = [&] {
    cols.clear();
    if (arrange && customCardLayout()) {
      cols.resize(3);
      for (int c = 0; c < 3; ++c)
        for (auto& id : cardCols_[c])
          if (cardSec(id) >= 0 && shown[cardSec(id)]) cols[c].push_back(cardSec(id));
      return;
    }
    std::vector<int> ends;
    best(0, std::min({3, fitK, n}), ends);
    int start = 0;
    for (int e : ends) {
      cols.emplace_back(secs.begin() + start, secs.begin() + e);
      start = e;
    }
  };
  buildCols();
  int k = (int)cols.size();
  float colW = (gw - gap * (k - 1)) / k;

  // ---- card drag ----
  int dragSec = cardDrag_.empty() ? -1 : cardSec(cardDrag_);
  if (dragSec >= 0 && (!arrange || !shown[dragSec])) {
    cancelCardDrag();
    dragSec = -1;
  }
  if (!arrange || !io.MouseDown[0]) cardPress_.clear();
  int dropCol = -1, dropIdx = -1;
  if (dragSec >= 0) {
    for (auto& c : cols) c.erase(std::remove(c.begin(), c.end(), dragSec), c.end());
    // Drop target: the column under the pointer, before the first card whose
    // middle is below the pointer.
    ImVec2 m = io.MousePos - origin;
    dropCol = std::clamp((int)std::floor((m.x + gap / 2) / (colW + gap)), 0, k - 1);
    dropIdx = 0;
    float y = 0;
    for (int s : cols[dropCol]) {
      if (m.y <= y + hOf(s) / 2) break;
      ++dropIdx;
      y += hOf(s) + gap;
    }
    if (!io.MouseDown[0]) {
      // Drop: the first arrangement starts from the automatic layout.
      std::vector<std::vector<std::string>> vis(3);
      for (int c = 0; c < k; ++c)
        for (int s : cols[c]) vis[c].push_back(kCardIds[s]);
      bool wasCustom = customCardLayout();
      if (!wasCustom) {
        buildCols();
        for (int c = 0; c < 3; ++c) {
          cardCols_[c].clear();
          for (int s : cols[c]) cardCols_[c].push_back(kCardIds[s]);
        }
      }
      std::vector<std::string> before[3] = {cardCols_[0], cardCols_[1], cardCols_[2]};
      moveCard(cardDrag_, dropCol, dropIdx, vis);
      bool same = true;
      for (int c = 0; c < 3; ++c) same &= before[c] == cardCols_[c];
      if (same && !wasCustom)
        for (auto& c : cardCols_) c.clear();  // dropped where it was: stay automatic
      else if (!same)
        saveCardLayout();
      // The card settles from where it was dropped into its slot.
      CardAnim& a = cardAnim_[cardDrag_];
      a.cur = a.from = cardGhostRel_;
      a.to = ImVec2(-1e6f, -1e6f);
      a.t0 = now;
      cardAnimUntil_ = now + 0.3;
      cardDrag_.clear();
      dragSec = -1;
      dropCol = dropIdx = -1;
      buildCols();
    } else {
      // Near the top / bottom edge the panel scrolls along.
      ImVec2 wmin = ImGui::GetWindowPos(), wmax = wmin + ImGui::GetWindowSize();
      float edge = 48 * S, sp = 1100 * S * std::clamp(io.DeltaTime, 0.f, 0.05f);
      if (io.MousePos.y < wmin.y + edge)
        ImGui::SetScrollY(std::max(0.f, ImGui::GetScrollY() - sp * (wmin.y + edge - io.MousePos.y) / edge));
      else if (io.MousePos.y > wmax.y - edge)
        ImGui::SetScrollY(std::min(ImGui::GetScrollMaxY(),
                                   ImGui::GetScrollY() + sp * (io.MousePos.y - (wmax.y - edge)) / edge));
    }
  }

  // Slots: each card's place (relative to the grid origin); sec -1 = the
  // drop slot of the card being dragged.
  struct Slot {
    int sec;
    float x, y, h;
    bool last;
  };
  std::vector<Slot> slots;
  float maxH = 0;
  for (int c = 0; c < k; ++c) {
    std::vector<int> items = cols[c];
    if (c == dropCol) items.insert(items.begin() + std::min(dropIdx, (int)items.size()), -1);
    float y = 0;
    for (size_t i = 0; i < items.size(); ++i) {
      float h = hOf(items[i] < 0 ? dragSec : items[i]);
      slots.push_back({items[i], c * (colW + gap), y, h, i + 1 == items.size()});
      y += h + gap;
    }
    maxH = std::max(maxH, y - gap);
  }
  // Position of a card: eases (cubic, 150 ms) to a new slot while dragging
  // and right after a drop / reset; otherwise it snaps.
  const bool animate = dragSec >= 0 || now < cardAnimUntil_;
  auto place = [&](const std::string& key, ImVec2 target) {
    auto it = cardAnim_.find(key);
    if (it == cardAnim_.end() || !animate) {
      CardAnim a;
      a.from = a.to = a.cur = target;
      a.t0 = now;
      cardAnim_[key] = a;
      return target;
    }
    CardAnim& a = it->second;
    if (std::fabs(target.x - a.to.x) > 0.5f || std::fabs(target.y - a.to.y) > 0.5f) {
      a.from = a.cur;
      a.to = target;
      a.t0 = now;
    }
    float t = std::clamp((float)((now - a.t0) / 0.15), 0.f, 1.f);
    float e = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
    a.cur = a.from + (a.to - a.from) * e;
    return a.cur;
  };

  TextStyle secTitle = tk::section();
  TextStyle rowLabel = tk::label();
  float cw = 0;  // content width of the current card
  auto gapY = [&](float h) { ImGui::Dummy(ImVec2(cw, h)); };
  // The card being drawn: its id, top-left corner, whether it is the lifted
  // ghost of a drag, and whether its header strip is hovered.
  int curSec = -1;
  ImVec2 curCard0;
  bool curGhost = false, curHdrHover = false;
  bool curFolded = false;  // draw only the header (the card is collapsed)
  float curFoldAmt = 0;    // 0 expanded … 1 collapsed (animated), for the chevron
  // Fold animation: 0 … 1 since the card was last collapsed / expanded.
  auto foldProgress = [&](const std::string& id) {
    auto it = cardFoldT0_.find(id);
    return it == cardFoldT0_.end() ? 1.f : std::clamp((float)((now - it->second) / 0.22), 0.f, 1.f);
  };
  // Card header: icon, title, optional value at the right, an optional
  // on / off switch (sw = 0 / 1; -1 = none) and the padlock left of it (when
  // cards can be arranged). Returns true when the switch was clicked. An
  // unlocked card is dragged by this strip.
  auto header = [&](const char* icon, const std::string& title, const std::string& meta = "", int sw = -1) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float lh = std::max(lineHeightPx(secTitle), 20 * S);
    gradientIcon(dl, icon, ImVec2(p.x + 9 * S, p.y + lh / 2), 18 * S, fbScale_);
    drawText(dl, ImVec2(p.x + 26 * S, std::round(p.y + (lh - lineHeightPx(secTitle)) / 2)), secTitle, kFg, title);
    float right = p.x + cw;
    bool toggled = false;
    const std::string cid = curSec >= 0 ? kCardIds[curSec] : "";
    const bool unlocked = cardsUnlocked_.count(cid) > 0;
    const float bs = std::round(24 * S);
    auto smallGlass = [&](ImVec2 b0, ImVec2 b1, bool hv) {
      GlassStyle g;
      g.solid = 0.2f;
      g.shadowAlpha = 0.2f;
      g.lens = false;
      if (hv) {
        g.tint = kInk;
        g.tintAmount = 0.1f;
      }
      glass(dl, b0, b1, bs / 2, g);
    };
    if (curSec >= 0) {
      // Fold button (rightmost): a chevron that points up while the card is
      // open (collapse) and turns to point down when it is collapsed (expand).
      ImVec2 f1(right, std::round(p.y + lh / 2 + bs / 2)), f0(f1.x - bs, f1.y - bs);
      bool fhv = false;
      std::string fid = "##cardfold" + cid;
      bool folded = cardsFolded_.count(cid) > 0;
      if (hit(fid.c_str(), f0, f1, true, &fhv) && !curGhost && dragSec < 0) setCardFolded(cid, !folded);
      smallGlass(f0, f1, fhv);
      ImVec2 fc = (f0 + f1) * 0.5f;
      int v0 = dl->VtxBuffer.Size;
      icons::draw(dl, "chevron", fc, std::round(14 * S), fhv ? kFg : kMuted, fbScale_);
      float ang = 3.14159265f * (1.f - curFoldAmt), cs = std::cos(ang), sn = std::sin(ang);
      for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
        ImVec2 d = dl->VtxBuffer[i].pos - fc;
        dl->VtxBuffer[i].pos = ImVec2(fc.x + d.x * cs - d.y * sn, fc.y + d.x * sn + d.y * cs);
      }
      if (fhv && dragSec < 0) {
        tooltip_ = i18n::tr(L, folded ? "settings.expandCard" : "settings.collapseCard");
        tooltipAnchor_ = ImVec2(fc.x, f1.y + 8 * S);
        tooltipBelow_ = true;
      }
      right = f0.x - 6 * S;
    }
    float stripEnd = right;
    if (arrange && curSec >= 0) {
      // Padlock: a small glass button like the panel's close button.
      ImVec2 l1(right, std::round(p.y + lh / 2 + bs / 2)), l0(l1.x - bs, l1.y - bs);
      bool lhv = false;
      std::string lid = "##cardlock" + cid;
      if (hit(lid.c_str(), l0, l1, true, &lhv) && !curGhost) setCardUnlocked(cid, !unlocked);
      smallGlass(l0, l1, lhv);
      ImVec2 lc = (l0 + l1) * 0.5f;
      if (unlocked) {  // open padlock in the brand gradient, with a gradient rim
        gradientBorder(dl, l0, l1, bs / 2, 1.25f * S, 0.75f);
        gradientIcon(dl, "unlock", lc, std::round(14 * S), fbScale_);
      } else
        icons::draw(dl, "lock", lc, std::round(14 * S), lhv ? kFg : kMuted, fbScale_);
      if (lhv && dragSec < 0) {
        tooltip_ = i18n::tr(L, unlocked ? "settings.lockCard" : "settings.unlockCard");
        tooltipAnchor_ = ImVec2(lc.x, l1.y + 8 * S);
        tooltipBelow_ = true;
      }
      right = l0.x - 10 * S;
    }
    if (sw >= 0) {
      std::string id = "##cardsw" + std::string(icon);
      toggled = toggleSwitch(id.c_str(), sw != 0, ImVec2(right - 36 * S, p.y + (lh - 20 * S) / 2));
      right -= 46 * S;
    }
    stripEnd = right + 4 * S;
    if (!meta.empty()) {
      TextStyle mt = tk::value();
      std::string m = ellipsize(mt, meta, right - (p.x + 26 * S + textWidth(secTitle, title) + 12 * S));
      float w = textWidth(mt, m);
      drawText(dl, ImVec2(right - w, std::round(p.y + (lh - lineHeightPx(mt)) / 2)), mt, kMuted, m);
    }
    // Unlocked: the header strip (card top edge to just under the title,
    // left of the padlock) starts a drag after a few pixels of movement, so
    // a plain click does nothing and the card's own controls never drag it.
    if (arrange && unlocked && !curGhost && dragSec < 0) {
      ImVec2 s0 = curCard0, s1(stripEnd, p.y + lh + 6 * S);
      ImGui::SetCursorScreenPos(s0);
      std::string did = "##carddrag" + cid;
      ImGui::InvisibleButton(did.c_str(), ImVec2(std::max(1.f, s1.x - s0.x), std::max(1.f, s1.y - s0.y)));
      bool hv = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
      if (ImGui::IsItemActivated()) {
        cardPress_ = cid;
        cardPressPos_ = io.MousePos;
      }
      if (hv || act) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        curHdrHover = true;
      }
      ImVec2 d = io.MousePos - cardPressPos_;
      if (act && cardPress_ == cid && d.x * d.x + d.y * d.y > 16 * S * S) {
        cardDrag_ = cid;
        cardGrab_ = cardPressPos_ - curCard0;
        cardGhostRel_ = curCard0 - origin;
        cardDragT0_ = now;
        cardPress_.clear();
        cardAnim_.erase("#slot");
      }
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(cw, lh));
    gapY(14 * S);
    return toggled;
  };
  auto label = [&](const std::string& s, float w) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    drawText(dl, p, rowLabel, kMuted, ellipsize(rowLabel, s, w));
    ImGui::Dummy(ImVec2(w, lineHeightPx(rowLabel)));
    ImGui::Dummy(ImVec2(w, 6 * S));
  };
  // Label + dropdown at the cursor, `w` wide. Leaves the cursor below it.
  auto selectAt = [&](const std::string& lab, const char* id, const std::vector<Opt>& opts, std::string& value,
                      float w) {
    label(lab, w);
    std::string out;
    bool opened = false;
    bool ch = selectBox(id, opts, value, out, w, &opened);
    if (opened) anyComboOpen_ = true;
    if (ch) value = out;
    return ch;
  };
  auto selectRow = [&](const std::string& lab, const char* id, const std::vector<Opt>& opts, std::string& value) {
    bool ch = selectAt(lab, id, opts, value, cw);
    gapY(12 * S);
    return ch;
  };
  // Two dropdowns side by side.
  auto pairRow = [&](const std::function<void(float)>& left, const std::function<void(float)>& right) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float hw = (cw - 10 * S) / 2;
    ImGui::BeginGroup();
    left(hw);
    ImGui::EndGroup();
    float y1 = ImGui::GetItemRectMax().y;
    ImGui::SetCursorScreenPos(ImVec2(p.x + hw + 10 * S, p.y));
    ImGui::BeginGroup();
    right(hw);
    ImGui::EndGroup();
    float y2 = ImGui::GetItemRectMax().y;
    ImGui::SetCursorScreenPos(ImVec2(p.x, std::max(y1, y2)));
    gapY(12 * S);
  };
  auto segRow = [&](const std::string& lab, const char* id, const std::vector<Opt>& opts, std::string& value) {
    label(lab, cw);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 30 * S;
    bool ch = segmented(id, opts, value, p, cw, h);
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(cw, h));
    gapY(12 * S);
    return ch;
  };
  auto note = [&](const std::string& s, ImU32 col) {
    TextStyle hint = ts(Family::Plex, 400, 12, 0, false, 1.45f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float hh = drawWrapped(dl, ImVec2(p.x, p.y - 6 * S), cw, hint, col, s);
    ImGui::Dummy(ImVec2(cw, std::max(0.f, hh - 6 * S)));
    gapY(12 * S);
  };

  // Label + on / off switch on one line.
  auto switchRow = [&](const std::string& lab, const char* id, bool on) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = std::round(22 * S);
    TextStyle t = tk::body();
    drawText(dl, ImVec2(p.x, std::round(p.y + (h - lineHeightPx(t)) / 2)), t, kFg, ellipsize(t, lab, cw - 46 * S));
    bool ch = toggleSwitch(id, on, ImVec2(p.x + cw - 36 * S, p.y + (h - 20 * S) / 2));
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(cw, h));
    gapY(12 * S);
    return ch;
  };
  // Label and value on one line, slider below.
  auto sliderRow = [&](const std::string& lab, const char* id, float& v, float mn, float mx, float step,
                       const std::string& valueText) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    TextStyle vt = tk::value();
    float lh = lineHeightPx(rowLabel), vw = textWidth(vt, valueText);
    drawText(dl, p, rowLabel, kMuted, ellipsize(rowLabel, lab, cw - vw - 12 * S));
    drawText(dl, ImVec2(p.x + cw - vw, std::round(p.y + (lh - lineHeightPx(vt)) / 2)), vt, kFg, valueText);
    const float sh = std::round(20 * S);
    bool ch = rangeSlider(id, v, mn, mx, step, ImVec2(p.x, p.y + lh + 4 * S), cw, sh);
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(cw, lh + 4 * S + sh));
    gapY(10 * S);
    return ch;
  };
  // Full-width neutral glass button.
  auto buttonRow = [&](const char* id, const std::string& lab, const char* icon, bool enabled) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = std::round(34 * S);
    bool c = glassButton(dl, id, p, ImVec2(p.x + cw, p.y + h), lab, icon, false, enabled, tk::buttonSmall());
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(cw, h));
    gapY(10 * S);
    return c;
  };

  auto drawSection = [&](int sec) {
    switch (sec) {
      case SecSource: {
        std::string meta;
        if (connected_ && streamActual_.width)
          meta = resolutionText(streamActual_.width, streamActual_.height) + " \xC2\xB7 " +
                 fpsLabel((int)std::lround(streamActual_.fps)) + " \xC2\xB7 " + fourccFriendly(streamActual_.fourcc);
        header("plug", i18n::tr(L, "source.title"), meta);
        if (curFolded) break;
        {
          std::vector<Opt> o{{"", i18n::tr(L, "common.automatic")}};
          for (auto& v : videoDevs_) o.push_back({v.id, v.name});
          std::string v = savedVideo_;
          if (selectRow(i18n::tr(L, "source.device"), "##srcdev", o, v)) setSourceDevice(v);
        }
        pairRow(
            [&](float w) {
              std::vector<Opt> o;
              for (auto& r : sourceResolutions())
                o.push_back({std::to_string(r.first) + "x" + std::to_string(r.second), resolutionLabel(r.first, r.second)});
              if (o.empty()) o.push_back({"", "\xE2\x80\x94"});
              std::string v = std::to_string(modeW_) + "x" + std::to_string(modeH_);
              if (selectAt(i18n::tr(L, "source.resolution"), "##srcres", o, v, w)) {
                int rw = 0, rh = 0;
                if (std::sscanf(v.c_str(), "%dx%d", &rw, &rh) == 2) setSourceResolution(rw, rh);
              }
            },
            [&](float w) {
              std::vector<Opt> o;
              for (int f : sourceFrameRates(modeW_, modeH_)) o.push_back({std::to_string(f), fpsLabel(f)});
              if (o.empty()) o.push_back({"", "\xE2\x80\x94"});
              std::string v = std::to_string(modeFps_);
              if (selectAt(i18n::tr(L, "source.frameRate"), "##srcfps", o, v, w) && !v.empty())
                setSourceFrameRate(std::atoi(v.c_str()));
            });
        {
          std::vector<Opt> o{{std::to_string(PREFER_AUTO), i18n::tr(L, "common.auto")},
                             {std::to_string(PREFER_UNCOMPRESSED), i18n::tr(L, "source.uncompressed")},
                             {std::to_string(PREFER_COMPRESSED), i18n::tr(L, "source.compressed")}};
          std::string v = std::to_string(formatPref_);
          if (segRow(i18n::tr(L, "source.format"), "##srcfmt", o, v)) setSourceFormat((uint32_t)std::atoi(v.c_str()));
          note(i18n::tr(L, "source.formatHint"), kMuted);
        }
        break;
      }
      case SecSound: {
        header("audio", i18n::tr(L, "sound.title"));
        if (curFolded) break;
        {
          std::vector<Opt> o{{"", i18n::tr(L, "common.automatic")}};
          for (auto& d : audioIns_)
            o.push_back({d.id, d.id == kUsbCardAudio ? std::string(i18n::tr(L, "sound.usbCard")) : d.id});
          std::string v = savedAudioIn_;
          if (selectRow(i18n::tr(L, "sound.input"), "##sndin", o, v)) setGameAudioInput(v);
        }
        {
          std::vector<Opt> o{{"default", i18n::tr(L, "sound.systemDefault")}};
          for (auto& d : audioOuts_)
            if (d.id != "default") o.push_back({d.id, d.id});
          std::string v = selAudioOut_.empty() ? "default" : selAudioOut_;
          if (selectRow(i18n::tr(L, "sound.output"), "##sndout", o, v)) setAudioOutput(v);
        }
        {
          std::vector<Opt> o{{"", i18n::tr(L, "common.automatic")}};
          for (auto& d : audioIns_)
            if (d.id != kUsbCardAudio) o.push_back({d.id, d.id});
          std::string v = micSource_;
          if (selectRow(i18n::tr(L, "sound.mic"), "##sndmic", o, v)) setMicSource(v);
        }
        {
          float v = volume_;
          if (sliderRow(i18n::tr(L, "sound.listenVolume"), "##sndvol", v, 0.f, 1.f, 0.01f,
                        std::to_string((int)std::lround(volume_ * 100)) + "%"))
            setListenVolume(v);
        }
        break;
      }
      case SecDepth: {
        header("depth", i18n::tr(L, "depth.title"));
        if (curFolded) break;
        {
          std::vector<Opt> o;
          for (auto& f : depthFormatOptions()) o.push_back({std::to_string(f.first), f.second});
          std::string v = std::to_string(depthFormat());
          if (selectRow(i18n::tr(L, "depth.format"), "##depthfmt", o, v)) setDepthFormat(std::atoi(v.c_str()));
        }
        {
          float v = depthStrength_;
          if (sliderRow(i18n::tr(L, "depth.strength"), "##depthstrset", v, 0.1f, 1.f, 0.05f,
                        std::to_string((int)std::lround(depthStrength_ * 100)) + "%"))
            setDepthStrength(v);
        }
        if (depth3d_ && stereoOut_ != StereoOff) note(i18n::tr(L, "stereo.hint"), kMuted);
        break;
      }
      case SecPicture: {
        header("droplet", i18n::tr(L, "picture.title"));
        if (curFolded) break;
        const char* names[3] = {"picture.brightness", "picture.contrast", "picture.saturation"};
        const int which[3] = {PicBrightness, PicContrast, PicSaturation};
        for (int i = 0; i < 3; ++i) {
          float v = (float)pictureValue(which[i]);
          std::string id = std::string("##pic") + std::to_string(i);
          if (sliderRow(i18n::tr(L, names[i]), id.c_str(), v, -40.f, 40.f, 1.f, signedValue(pictureValue(which[i]))))
            setPictureValue(which[i], (int)std::lround(v));
        }
        {
          float v = color_.gamma;
          char g[16];
          std::snprintf(g, sizeof g, "%.2f", color_.gamma);
          if (sliderRow(i18n::tr(L, "picture.gamma"), "##picgamma", v, kGammaMin, kGammaMax, 0.01f, g)) setGamma(v);
        }
        {
          label(i18n::tr(L, "picture.presets"), cw);
          ImVec2 p = ImGui::GetCursorScreenPos();
          TextStyle ct = tk::chip();
          const float ch = std::round(28 * S), gx = 6 * S;
          float x = p.x, y = p.y;
          int active = activePicturePreset();
          for (int i = 0; i < kPicturePresets; ++i) {
            std::string t = i18n::tr(L, picturePreset(i).key);
            float w = std::round(textWidth(ct, t) + 26 * S);
            if (x > p.x && x + w > p.x + cw) {
              x = p.x;
              y += ch + gx;
            }
            ImVec2 a0(x, y), a1(x + w, y + ch);
            bool hv = false;
            std::string id = "##preset" + std::to_string(i);
            if (hit(id.c_str(), a0, a1, true, &hv)) applyPicturePreset(i);
            GlassStyle gs;
            gs.lens = false;
            gs.shadowAlpha = 0.12f;
            if (i == active) {
              gs.gradientTint = true;
              gs.tintAmount = 0.86f;
            } else {
              gs.solid = 0.15f;
              if (hv) {
                gs.tint = kInk;
                gs.tintAmount = 0.08f;
              }
            }
            glass(dl, a0, a1, ch / 2, gs);
            drawText(dl, ImVec2(x + 13 * S, std::round(y + (ch - lineHeightPx(ct)) / 2)), ct,
                     i == active ? IM_COL32_WHITE : kFg, t);
            x += w + gx;
          }
          ImGui::SetCursorScreenPos(p);
          ImGui::Dummy(ImVec2(cw, y + ch - p.y));
          gapY(14 * S);
        }
        {
          // Retro look: Off / CRT / VHS / Arcade (same as the dock tile, C).
          static const char* names[kRetroStyles + 1] = {"common.off", "retro.crt", "retro.vhs", "retro.arcade"};
          std::vector<Opt> o;
          for (int i = 0; i <= kRetroStyles; ++i) o.push_back({std::to_string(i), i18n::tr(L, names[i])});
          std::string v = std::to_string(retroStyle_);
          if (segRow(i18n::tr(L, "retro.title"), "##retroseg", o, v)) setRetroStyle(std::atoi(v.c_str()));
        }
        {
          // 4K upscaling: Off or one of the upscalers (choosing one turns
          // upscaling on, like the dock popover; Off keeps the choice).
          std::vector<Opt> o{{"off", i18n::tr(L, "common.off")}};
          for (int i = 0; i < kUpscaleModes; ++i) o.push_back({std::to_string(i), kUpscalerShort[i]});
          static const char* descs[kUpscaleModes] = {"upscale.fsrDesc", "upscale.anime4kDesc", "upscale.lanczosDesc",
                                                     "upscale.nisDesc"};
          std::string v = upscale_ ? std::to_string(upscaleMode_) : "off";
          if (segRow(i18n::tr(L, "upscale.title"), "##upmode", o, v)) {
            if (v == "off") {
              setUpscale(false);
            } else {
              setUpscaleMode(std::atoi(v.c_str()));
              setUpscale(true);
            }
          }
          const char* desc = upscale_ ? descs[std::clamp(upscaleMode_, 0, kUpscaleModes - 1)] : "upscale.offDesc";
          note(i18n::tr(L, desc), kMuted);
        }
        break;
      }
      case SecOverlay: {
        if (header("webcam", i18n::tr(L, "overlay.title"), "", overlayOn_ ? 1 : 0)) setOverlayOn(!overlayOn_);
        if (curFolded) break;
        {
          std::vector<Opt> o{{"", i18n::tr(L, "common.automatic")}};
          for (auto& v : videoDevs_)
            if (v.id != selVideo_) o.push_back({v.id, v.name});
          std::string v = savedOverlayCam_;
          if (selectRow(i18n::tr(L, "overlay.camera"), "##ovcam", o, v)) setOverlayCamera(v);
        }
        {
          std::vector<Opt> o;
          for (int i = 0; i < kOverlayQualities; ++i) o.push_back({std::to_string(i), overlayQualityLabel(i)});
          std::string v = std::to_string(overlayQuality_);
          if (selectRow(i18n::tr(L, "overlay.quality"), "##ovq", o, v)) setOverlayQuality(std::atoi(v.c_str()));
        }
        pairRow(
            [&](float w) {
              label(i18n::tr(L, "overlay.size"), w);
              ImVec2 p = ImGui::GetCursorScreenPos();
              const float h = std::round(32 * S);
              std::vector<Opt> o{{"0", "S"}, {"1", "M"}, {"2", "L"}};
              std::string v = std::to_string(overlaySize_);
              if (segmented("##ovsizeseg", o, v, p, w, h)) setOverlaySize(std::atoi(v.c_str()));
              ImGui::SetCursorScreenPos(p);
              ImGui::Dummy(ImVec2(w, h));
            },
            [&](float w) {
              label(i18n::tr(L, "overlay.corner"), w);
              ImVec2 p = ImGui::GetCursorScreenPos();
              const float h = std::round(32 * S), pw = std::round(h * 16.f / 9.f);
              int c = cornerPicker("##ovcornerset", overlayCorner_, p, pw, h);
              if (c >= 0) setOverlayCorner(c);
              TextStyle vt = tk::label();
              drawText(dl, ImVec2(p.x + pw + 10 * S, std::round(p.y + (h - lineHeightPx(vt)) / 2)), vt, kMuted,
                       ellipsize(vt, i18n::tr(L, cornerKey(overlayCorner_)), w - pw - 10 * S));
              ImGui::SetCursorScreenPos(p);
              ImGui::Dummy(ImVec2(w, h));
            });
        if (switchRow(i18n::tr(L, "overlay.mirror"), "##ovmirrorset", overlayMirror_)) setOverlayMirror(!overlayMirror_);
        if (buttonRow("##ovswapset", i18n::tr(L, "overlay.swap"), "swap", canSwapPictures())) swapPictures();
        note(i18n::tr(L, "overlay.swapHint"), kMuted);
        break;
      }
      case SecVr: {
        header("vr", "VR");
        if (curFolded) break;
        {
          std::vector<Opt> o{{"1", i18n::tr(L, "vr.passthrough")}, {"0", i18n::tr(L, "vr.darkRoom")}};
          std::string v = vrPassthrough_ ? "1" : "0";
          if (segRow(i18n::tr(L, "vr.background"), "##vrbg", o, v)) {
            vrPassthrough_ = v == "1";
            settings_.setNum("vr.passthrough", vrPassthrough_ ? 1 : 0);
          }
        }
        {
          std::vector<Opt> o{{"0", i18n::tr(L, "vr.flat")}, {"1", i18n::tr(L, "vr.curved")}};
          std::string v = vrCurved_ ? "1" : "0";
          if (segRow(i18n::tr(L, "vr.screenShape"), "##vrshape", o, v)) {
            vrCurved_ = v == "1";
            settings_.setNum("vr.curved", vrCurved_ ? 1 : 0);
          }
        }
        {
          const float sizes[] = {1.6f, 2.6f, 3.6f, 5.0f};
          const char* names[] = {i18n::tr(L, "vr.sizeSmall"), i18n::tr(L, "vr.sizeMedium"), i18n::tr(L, "vr.sizeLarge"),
                                 i18n::tr(L, "vr.sizeHuge")};
          std::vector<Opt> o;
          int cur = 0;
          for (int i = 0; i < 4; ++i) {
            o.push_back({std::to_string(i), names[i]});
            if (std::fabs(sizes[i] - vrScreenWidth_) < std::fabs(sizes[cur] - vrScreenWidth_)) cur = i;
          }
          std::string v = std::to_string(cur);
          if (segRow(i18n::tr(L, "vr.screenSize"), "##vrsize", o, v))
            setVrScreen(sizes[std::atoi(v.c_str())], vrScreenDist_, true);
        }
        {
          const float dists[] = {1.5f, 2.2f, 3.2f, 5.0f};
          const char* names[] = {i18n::tr(L, "vr.distNear"), i18n::tr(L, "vr.distMedium"), i18n::tr(L, "vr.distFar"),
                                 i18n::tr(L, "vr.distVeryFar")};
          std::vector<Opt> o;
          int cur = 0;
          for (int i = 0; i < 4; ++i) {
            o.push_back({std::to_string(i), names[i]});
            if (std::fabs(dists[i] - vrScreenDist_) < std::fabs(dists[cur] - vrScreenDist_)) cur = i;
          }
          std::string v = std::to_string(cur);
          if (segRow(i18n::tr(L, "vr.screenDistance"), "##vrdist", o, v))
            setVrScreen(vrScreenWidth_, dists[std::atoi(v.c_str())], true);
          note(i18n::tr(L, "vr.dragHint"), kMuted);
        }
        {
          std::vector<Opt> o{{"72", "72 Hz"}, {"90", "90 Hz"}, {"120", "120 Hz"}};
          std::string v = std::to_string(vrRefresh_);
          if (segRow(i18n::tr(L, "vr.refreshRate"), "##vrhz", o, v)) {
            vrRefresh_ = std::atoi(v.c_str());
            settings_.setNum("vr.refresh", vrRefresh_);
          }
        }
        if (buttonRow("##vrpanelreset", i18n::tr(L, "vr.resetPanel"), "vr", vrPanelMoved())) resetVrPanel();
        break;
      }
      case SecDisplay: {
        header("fullscreen", i18n::tr(L, "display.title"));
        if (curFolded) break;
        {
          std::vector<Opt> o{{"auto", i18n::tr(L, "common.auto")}, {"dark", i18n::tr(L, "display.themeDark")},
                             {"light", i18n::tr(L, "display.themeLight")}};
          std::string v = themePref_;
          if (segRow(i18n::tr(L, "display.theme"), "##theme", o, v)) {
            themePref_ = v;
            settings_.set("ui.theme", themePref_);
          }
          if (themePref_ == "auto") note(i18n::tr(L, "display.themeAutoHint"), kMuted);
        }
        {
          char autoLabel[64];
          std::snprintf(autoLabel, sizeof autoLabel, "%s (%d%%)", i18n::tr(L, "common.auto"),
                        (int)std::lround(autoScale_ * 100));
          std::vector<Opt> o{{"0", autoLabel}};
          for (int p : {100, 125, 150, 175, 200, 250, 300}) o.push_back({std::to_string(p), std::to_string(p) + "%"});
          std::string v = std::to_string(uiScalePct_);
          if (selectRow(i18n::tr(L, "display.interfaceSize"), "##uiscale", o, v)) {
            uiScalePct_ = std::atoi(v.c_str());
            uiZoom_ = 1.f;
            settings_.setNum("ui.scale", uiScalePct_);
            settings_.setNum("ui.zoom", 1);
          }
        }
        break;
      }
      case SecGeneral: {
        header("folder", i18n::tr(L, "general.title"));
        if (curFolded) break;
        if (switchRow(i18n::tr(L, "general.autoConnect"), "##autoconnect", autoConnect_)) setAutoConnect(!autoConnect_);
        note(i18n::tr(L, "general.autoConnectHint"), kMuted);
        if (buttonRow("##opensnaps", i18n::tr(L, "general.openSnapshots"), "folder", true)) openSnapshotsFolder();
        if (buttonRow("##openrecs", i18n::tr(L, "general.openRecordings"), "folder", true)) openRecordingsFolder();
        break;
      }
      case SecAbout: {
        header("shield", i18n::tr(L, "about.title"), platform::versionLabel());
        if (curFolded) break;
        // The privacy note lives in the sheet behind the licences button; the
        // card only shows live load figures.
        TextStyle body = ts(Family::Plex, 400, 12, 0, false, 1.45f);
        if (connected_) {
          std::string load;
          double ms = videoTimer_.averageMs();
          if (xrMode_ && stereoTimer_.averageMs() >= 0 && depth3d_) ms = (ms < 0 ? 0 : ms) + stereoTimer_.averageMs();
          char b[160];
          if (ms >= 0) {
            std::snprintf(b, sizeof b, "%s: %.1f ms", i18n::tr(L, "about.gpuTime"), ms);
            load = b;
          }
          const GpuImage& v = pipe_.view();
          if (v.w > 0) {
            std::snprintf(b, sizeof b, "%s%s: %d×%d", load.empty() ? "" : " · ", i18n::tr(L, "about.processing"), v.w, v.h);
            load += b;
          }
          if (!load.empty()) {
            ImVec2 lp = ImGui::GetCursorScreenPos();
            float lh = drawWrapped(dl, lp, cw, body, kMuted, load);
            ImGui::Dummy(ImVec2(cw, lh));
            gapY(10 * S);
          }
        }
        TextStyle t = ts(Family::Manrope, 600, 12, 0.02f);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float bh = lineHeightPx(t) + 16 * S + 2;
        ImVec2 p1(p0.x + cw, p0.y + bh);
        bool hv;
        if (hit("##licenses", p0, p1, true, &hv)) {
          aboutOpen_ = true;
          settingsOpen_ = false;
        }
        GlassStyle gs;
        gs.solid = 0.2f;
        gs.shadowAlpha = 0.2f;
        gs.lens = false;
        if (hv) {
          gs.tint = kInk;
          gs.tintAmount = 0.08f;
        }
        glass(dl, p0, p1, bh / 2, gs);
        std::string lab = i18n::tr(L, "about.licenses");
        float lw = textWidth(t, lab);
        drawText(dl, ImVec2((p0.x + p1.x) / 2 - lw / 2, p0.y + 8 * S + 1), t, kFg, lab);
        ImGui::SetCursorScreenPos(p0);
        ImGui::Dummy(ImVec2(cw, bh));
        gapY(12 * S);
        break;
      }
    }
  };

  // ---- cards ----
  // While a card is dragged the others are inert (no hover, no clicks); they
  // only glide out of the way.
  const bool dragging = dragSec >= 0;
  if (dragging) {
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.f);
    ImGui::BeginDisabled();
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  }
  dl->ChannelsSplit(2);
  float contentBottom = origin.y;  // natural (unstretched) bottom
  float runY = 0;
  int runCol = -1;
  for (const Slot& sl : slots) {
    int c = (int)std::lround(sl.x / (colW + gap));
    if (c != runCol) {
      runCol = c;
      runY = 0;
    }
    ImVec2 rp = place(sl.sec < 0 ? std::string("#slot") : std::string(kCardIds[sl.sec]), ImVec2(sl.x, runY));
    float cx = origin.x + rp.x, y = origin.y + rp.y;
    if (sl.sec < 0) {
      // Drop slot: a soft brand-gradient wash with a dashed gradient outline,
      // as tall as the card being dragged.
      dl->ChannelsSetCurrent(0);
      ImVec2 a(cx, y), b(cx + colW, y + sl.h);
      int v0 = dl->VtxBuffer.Size;
      dl->AddRectFilled(a, b, withAlpha(IM_COL32_WHITE, 0.1f), 18 * S);
      float in = 1.f * S;
      dashedRoundRect(dl, a + ImVec2(in, in), b - ImVec2(in, in), 17 * S, 1.75f * S, 8 * S, 6 * S, IM_COL32_WHITE);
      shadeGradient(dl, v0, dl->VtxBuffer.Size, a, b, 0.95f);
      contentBottom = std::max(contentBottom, y + sl.h);
      runY += sl.h + gap;
      continue;
    }
    dl->ChannelsSetCurrent(1);
    cw = colW - 2 * cpad;
    curSec = sl.sec;
    curCard0 = ImVec2(cx, y);
    curGhost = false;
    curHdrHover = false;
    // Collapsing / expanding: the whole card is drawn, clipped to a height
    // that eases between its full height and its header; when collapsed only
    // the header is drawn.
    const std::string cid = kCardIds[sl.sec];
    const bool folded = cardsFolded_.count(cid) > 0;
    const float fp = foldProgress(cid), fe = 1.f - (1.f - fp) * (1.f - fp) * (1.f - fp);
    const bool foldAnim = fp < 1.f;
    const float hdrOnly = std::max(lineHeightPx(secTitle), 20 * S) + 2 * S + 2 * cpad;
    curFolded = folded && !foldAnim;
    curFoldAmt = folded ? fe : 1.f - fe;
    float clipH = folded ? fullH[sl.sec] * S + (hdrOnly - fullH[sl.sec] * S) * fe
                         : hdrOnly + (fullH[sl.sec] * S - hdrOnly) * fe;
    if (foldAnim) dl->PushClipRect(ImVec2(cx, y), ImVec2(cx + colW, y + clipH), true);
    ImGui::SetCursorScreenPos(ImVec2(cx + cpad, y + cpad));
    ImGui::BeginGroup();  // new lines start at the card's left edge
    drawSection(sl.sec);
    ImGui::EndGroup();
    if (foldAnim) dl->PopClipRect();
    ImGui::SetCursorScreenPos(ImVec2(cx + cpad, ImGui::GetItemRectMax().y));
    float h = ImGui::GetCursorScreenPos().y - 12 * S - y + cpad;  // drop the trailing row gap
    if (!curFolded) fullH[sl.sec] = h / S;
    if (foldAnim) h = folded ? h + (hdrOnly - h) * fe : hdrOnly + (h - hdrOnly) * fe;
    curFolded = false;
    lastH[sl.sec] = h / S;
    float hb = h;  // every card is exactly as tall as its content
    dl->ChannelsSetCurrent(0);
    ImVec2 a(cx, y), b(cx + colW, y + hb);
    dl->AddRectFilled(a, b, ink(0.045f), 18 * S);
    if (arrange && cardsUnlocked_.count(kCardIds[sl.sec])) {
      // Unlocked: a faint dashed gradient outline (stronger over the header).
      int v0 = dl->VtxBuffer.Size;
      float in = 0.75f * S;
      dashedRoundRect(dl, a + ImVec2(in, in), b - ImVec2(in, in), 18 * S - in, 1.25f * S, 6 * S, 5 * S,
                      IM_COL32_WHITE);
      shadeGradient(dl, v0, dl->VtxBuffer.Size, a, b, curHdrHover ? 0.95f : 0.6f);
    } else {
      dl->AddRect(a, b, ink(0.07f), 18 * S, 0, 1.f);
    }
    contentBottom = std::max(contentBottom, y + h);
    runY += h + gap;
  }
  dl->ChannelsMerge();
  if (dragging) {
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    // The dragged card follows the pointer as a lifted glass ghost (kept
    // inside the visible grid): deeper shadow, gradient rim, ~3 % larger.
    float h = hOf(dragSec);
    ImVec2 wmin = ImGui::GetWindowPos(), wmax = wmin + ImGui::GetWindowSize();
    ImVec2 gp = io.MousePos - cardGrab_;
    gp.x = std::clamp(gp.x, origin.x, std::max(origin.x, origin.x + gw - colW));
    gp.y = std::clamp(gp.y, wmin.y, std::max(wmin.y, wmax.y - std::min(h, wmax.y - wmin.y)));
    cardGhostRel_ = gp - origin;
    float lift = std::clamp((float)((now - cardDragT0_) / 0.15), 0.f, 1.f);
    lift = 1.f - (1.f - lift) * (1.f - lift) * (1.f - lift);
    dl->PushClipRect(vp->Pos, vp->Pos + vp->Size, false);
    int v0 = dl->VtxBuffer.Size;
    ImVec2 a = gp, b(gp.x + colW, gp.y + h);
    shadow(dl, a + ImVec2(0, 8 * S * lift), b + ImVec2(0, 8 * S * lift), 18 * S, 40 * S, 1.2f * lift);
    GlassStyle gs;
    gs.solid = 0.9f;
    gs.shadowAlpha = 0.4f;
    gs.lens = false;
    glass(dl, a, b, 18 * S, gs);
    dl->AddRectFilled(a, b, ink(0.06f), 18 * S);
    gradientBorder(dl, a, b, 18 * S, 1.5f * S, 0.55f + 0.4f * lift);
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.f);
    ImGui::BeginDisabled();
    cw = colW - 2 * cpad;
    curSec = dragSec;
    curCard0 = a;
    curGhost = true;
    curFolded = cardsFolded_.count(kCardIds[dragSec]) > 0;
    curFoldAmt = curFolded ? 1.f : 0.f;
    ImGui::SetCursorScreenPos(a + ImVec2(cpad, cpad));
    ImGui::BeginGroup();
    drawSection(dragSec);
    ImGui::EndGroup();
    curFolded = false;
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    curGhost = false;
    float sc = 1.f + 0.03f * lift;
    ImVec2 ctr = (a + b) * 0.5f;
    for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
      ImDrawVert& v = dl->VtxBuffer[i];
      v.pos = ctr + (v.pos - ctr) * sc;
    }
    dl->PopClipRect();
  }
  ImGui::SetCursorScreenPos(origin);
  ImGui::Dummy(ImVec2(gw, std::max(1.f, contentBottom - origin.y - 1.f)));
  ImGui::EndChild();
  // "Reset layout" (left of the close button): only while the user's own
  // card arrangement is in use (not in a window too narrow for it).
  if (arrange && customCardLayout()) {
    TextStyle bt = tk::buttonSmall();
    std::string lab = i18n::tr(L, "settings.resetLayout");
    float bw = std::round(textWidth(bt, lab) + 30 * S), bh = resetR1.y - resetTop;
    ImVec2 r0(resetR1.x - bw, resetTop);
    bool rhv = false;
    if (dragSec < 0 && hit("##resetlayout", r0, resetR1, true, &rhv)) resetCardLayout();
    GlassStyle rg;
    rg.solid = 0.2f;
    rg.shadowAlpha = 0.2f;
    rg.lens = false;
    if (rhv) {
      rg.tint = kInk;
      rg.tintAmount = 0.1f;
    }
    glass(wdl, r0, resetR1, bh / 2, rg);
    drawText(wdl, ImVec2(std::round(r0.x + 15 * S), std::round(resetTop + (bh - lineHeightPx(bt)) / 2)), bt, kFg, lab);
  }
  ImGui::PopStyleVar();  // ItemSpacing
  ui::suspendBackdrop(false);
  ImGui::End();
  ImGui::PopStyleVar();
}

// Language menu under the language pill (right-aligned to it).
void App::drawLangMenu() {
  float S = scale();
  const auto& langs = i18n::languages();
  TextStyle t = ts(Family::Plex, 500, 13);
  float rowH = 34 * S, padM = 6 * S;
  float W = langBtnMax_.x - langBtnMin_.x;
  for (auto& l : langs) W = std::max(W, 12 * S + textWidth(t, i18n::tr(l, "lang.name")) + 40 * S + 2 * padM);
  float H = padM * 2 + rowH * langs.size();
  ImVec2 pos(langBtnMax_.x - W, langBtnMax_.y + 8 * S);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(W, H));
  ImGui::Begin("##langmenu", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  popoverBg(dl, pos, pos + ImVec2(W, H), kPopover, 9 * S);
  langMenuMin_ = pos;
  langMenuMax_ = pos + ImVec2(W, H);
  float y = pos.y + padM;
  for (auto& l : langs) {
    ImVec2 a(pos.x + padM, y), b(pos.x + W - padM, y + rowH);
    bool hv = false;
    std::string id = "##lang_" + l;
    bool sel = l == lang_;
    if (hit(id.c_str(), a, b, true, &hv)) {
      if (!sel) setLang(l);
      langOpen_ = false;
    }
    if (hv) dl->AddRectFilled(a, b, ink(0.08f), 10 * S);
    std::string lab = i18n::tr(l, "lang.name");
    ImVec2 tp(a.x + 12 * S, (a.y + b.y) / 2 - lineHeightPx(t) / 2);
    if (sel)
      drawGradientText(dl, tp, t, lab);
    else
      drawText(dl, tp, t, kFg, lab);
    if (sel) gradientIcon(dl, "check", ImVec2(b.x - 16 * S, (a.y + b.y) / 2), 14 * S, fbScale_);
    y += rowH;
  }
  ImGui::End();
}

static const ImGuiWindowFlags kPopoverFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                              ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;

// Hover-popover window height: the drawn card `H` plus a bridge towards the
// tile, so the pointer can travel from the tile to the card. The window ends
// where the tiles' click areas begin (8 px above the tiles, `tileTop`), so it
// never covers a tile and a click on the tile always reaches it.
static float popoverWindowH(ImVec2 pos, float H, float tileTop) {
  float h = H + 14 * scale();
  if (tileTop > 0) h = std::min(h, tileTop - 8 * scale() - pos.y);
  return std::max(h, H);
}

// Listen popover: on / off, listen volume, and the output device (a link to
// Settings → Sound).
void App::drawListenPopover(ImVec2 anchor, bool& hovered) {
  float S = scale();
  const std::string& L = lang_;
  const float W = std::round(288 * S), pad = std::round(16 * S), cw = W - 2 * pad;
  const float rowH = std::round(22 * S), sliderH = std::round(22 * S), linkH = std::round(34 * S);
  TextStyle st = tk::section(), lab = tk::label(), val = tk::value();
  float H = std::round(pad + rowH + 14 * S + sliderH + 12 * S + 1 + 6 * S + linkH + 8 * S);
  ImVec2 pos = popoverPos(anchor, W, H);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(W, popoverWindowH(pos, H, dockTileTop_)));  // + hover bridge to the tile
  ImGui::Begin("##listenpop", nullptr, kPopoverFlags);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  popoverBg(dl, pos, pos + ImVec2(W, H), kPopover, 10 * S);
  float x = pos.x + pad, y = pos.y + pad;
  std::string title = i18n::tr(L, "listen.title");
  drawText(dl, ImVec2(x, std::round(y + (rowH - lineHeightPx(st)) / 2)), st, kFg, title);
  keycaps(dl, ImVec2(x + textWidth(st, title) + 8 * S, y + rowH / 2), {"L"}, true);
  if (toggleSwitch("##listenon", listen_, ImVec2(x + cw - 36 * S, y + (rowH - 20 * S) / 2))) setListen(!listen_);
  y += rowH + 14 * S;

  std::string pct = std::to_string((int)std::lround(volume_ * 100)) + "%";
  float vw = textWidth(val, "100%");
  float v = volume_;
  if (rangeSlider("##listenvol", v, 0.f, 1.f, 0.01f, ImVec2(x, y), cw - vw - 12 * S, sliderH)) setListenVolume(v);
  drawText(dl, ImVec2(x + cw - textWidth(val, pct), std::round(y + (sliderH - lineHeightPx(val)) / 2)), val,
           listen_ ? kFg : kMuted, pct);
  y += sliderH + 12 * S;
  dl->AddLine(ImVec2(x, y), ImVec2(x + cw, y), kBorder, 1.f);
  y += 1 + 6 * S;

  ImVec2 l0(x - 8 * S, y), l1(x + cw + 8 * S, y + linkH);
  bool hv = false;
  if (hit("##listenout", l0, l1, true, &hv)) {
    settingsOpen_ = true;
    langOpen_ = false;
  }
  if (hv) dl->AddRectFilled(l0, l1, ink(0.06f), tk::kFieldR * S);
  float cy = (l0.y + l1.y) / 2;
  std::string ol = i18n::tr(L, "listen.output");
  drawText(dl, ImVec2(x, std::round(cy - lineHeightPx(lab) / 2)), lab, kMuted, ol);
  std::string dev = selAudioOut_.empty() || selAudioOut_ == "default" ? i18n::tr(L, "sound.systemDefault") : selAudioOut_;
  float dx = x + textWidth(lab, ol) + 10 * S;
  drawText(dl, ImVec2(dx, std::round(cy - lineHeightPx(lab) / 2)), lab, kFg, ellipsize(lab, dev, x + cw - 22 * S - dx));
  icons::draw(dl, "line/arrow", ImVec2(x + cw - 7 * S, cy), 14 * S, hv ? kFg : kMuted, fbScale_);
  hovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemActive();
  ImGui::End();
}

// Webcam overlay popover: on / off, camera, quality, size, corner, mirror and
// "Swap pictures".
void App::drawOverlayPopover(ImVec2 anchor, bool& hovered) {
  float S = scale();
  const std::string& L = lang_;
  const float W = std::round(304 * S), pad = std::round(16 * S), cw = W - 2 * pad;
  const float rowH = std::round(22 * S), fieldH = std::round(30 * S), segH = std::round(32 * S),
              btnH = std::round(34 * S);
  TextStyle st = tk::section(), lab = tk::label(), body = tk::body();
  float labH = lineHeightPx(lab) + 6 * S;
  float H = std::round(pad + rowH + 14 * S + 2 * (labH + fieldH + 12 * S) + labH + segH + 14 * S + rowH + 14 * S + btnH +
                       pad);
  ImVec2 pos = popoverPos(anchor, W, H);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(W, popoverWindowH(pos, H, dockTileTop_)));  // + hover bridge to the tile
  ImGui::Begin("##overlaypop", nullptr, kPopoverFlags);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  popoverBg(dl, pos, pos + ImVec2(W, H), kPopover, 10 * S);
  bool comboOpen = false;
  float x = pos.x + pad, y = pos.y + pad;
  std::string title = i18n::tr(L, "overlay.title");
  drawText(dl, ImVec2(x, std::round(y + (rowH - lineHeightPx(st)) / 2)), st, kFg,
           ellipsize(st, title, cw - 36 * S - 40 * S));
  keycaps(dl, ImVec2(x + std::min(textWidth(st, title), cw - 76 * S) + 8 * S, y + rowH / 2), {"W"}, true);
  if (toggleSwitch("##ovon", overlayOn_, ImVec2(x + cw - 36 * S, y + (rowH - 20 * S) / 2))) setOverlayOn(!overlayOn_);
  y += rowH + 14 * S;

  auto field = [&](const char* key, const char* id, const std::vector<Opt>& opts, const std::string& value) {
    drawText(dl, ImVec2(x, y), lab, kMuted, i18n::tr(L, key));
    y += labH;
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    std::string out;
    bool opened = false;
    bool changed = selectBox(id, opts, value, out, cw, &opened);
    if (opened) comboOpen = true;
    y += fieldH + 12 * S;
    return changed ? out : std::string("\x01");
  };
  {
    std::vector<Opt> o{{"", i18n::tr(L, "common.automatic")}};
    for (auto& v : videoDevs_)
      if (v.id != selVideo_) o.push_back({v.id, v.name});
    std::string r = field("overlay.camera", "##popovcam", o, savedOverlayCam_);
    if (r != "\x01") setOverlayCamera(r);
  }
  {
    std::vector<Opt> o;
    for (int i = 0; i < kOverlayQualities; ++i) o.push_back({std::to_string(i), overlayQualityLabel(i)});
    std::string r = field("overlay.quality", "##popovq", o, std::to_string(overlayQuality_));
    if (r != "\x01") setOverlayQuality(std::atoi(r.c_str()));
  }
  {
    const float pw = std::round(segH * 16.f / 9.f), gapX = 16 * S, sw = cw - pw - gapX;
    drawText(dl, ImVec2(x, y), lab, kMuted, ellipsize(lab, i18n::tr(L, "overlay.size"), sw));
    std::string cl = i18n::tr(L, "overlay.corner");
    std::string cs = ellipsize(lab, cl, pw + gapX);
    drawText(dl, ImVec2(x + cw - textWidth(lab, cs), y), lab, kMuted, cs);
    y += labH;
    std::vector<Opt> o{{"0", "S"}, {"1", "M"}, {"2", "L"}};
    std::string v = std::to_string(overlaySize_);
    if (segmented("##popovsize", o, v, ImVec2(x, y), sw, segH)) setOverlaySize(std::atoi(v.c_str()));
    int c = cornerPicker("##popovcorner", overlayCorner_, ImVec2(x + cw - pw, y), pw, segH);
    if (c >= 0) setOverlayCorner(c);
    y += segH + 14 * S;
  }
  drawText(dl, ImVec2(x, std::round(y + (rowH - lineHeightPx(body)) / 2)), body, kFg,
           ellipsize(body, i18n::tr(L, "overlay.mirror"), cw - 46 * S));
  if (toggleSwitch("##popovmirror", overlayMirror_, ImVec2(x + cw - 36 * S, y + (rowH - 20 * S) / 2)))
    setOverlayMirror(!overlayMirror_);
  y += rowH + 14 * S;
  if (glassButton(dl, "##popovswap", ImVec2(x, y), ImVec2(x + cw, y + btnH), i18n::tr(L, "overlay.swap"), "swap", false,
                  canSwapPictures(), tk::buttonSmall()))
    swapPictures();
  if (comboOpen) anyComboOpen_ = true;
  hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || ImGui::IsAnyItemActive() || comboOpen;
  ImGui::End();
}
// 3D depth popover: the depth format (a dropdown that includes "No depth")
// and the strength, plus how to set up the display for the stereo formats.
std::vector<std::pair<int, std::string>> App::depthFormatOptions() const {
  const std::string& L = lang_;
  std::vector<std::pair<int, std::string>> o{{DepthNone, i18n::tr(L, "depth.none")}};
  o.push_back({DepthEffect, i18n::tr(L, "depth.effect")});
  o.push_back({DepthSbsFull, i18n::tr(L, "stereo.sbsFull")});
  o.push_back({DepthSbsHalf, i18n::tr(L, "stereo.sbsHalf")});
  o.push_back({DepthTab, i18n::tr(L, "stereo.topBottom")});
  o.push_back({DepthAnaglyph, i18n::tr(L, "stereo.anaglyph")});
  return o;
}

// Popover footer: a hairline, a muted note at the left and an action in the
// brand gradient at the right (e.g. "Free today: 12:34 left · Upgrade to
// Platinum"). Height: footerHeight(). Returns true when clicked.
static float footerHeight() { return std::round(10 * scale()) + lineHeightPx(tk::label()) + std::round(12 * scale()); }
static bool popoverFooter(ImDrawList* dl, const char* id, float x, float y, float w, const std::string& note,
                          const std::string& action) {
  float S = scale();
  TextStyle t = tk::label();
  dl->AddLine(ImVec2(x, y + 0.5f), ImVec2(x + w, y + 0.5f), kBorder);
  float ty = y + 10 * S;
  bool hv = false;
  bool clicked = hit(id, ImVec2(x, y), ImVec2(x + w, ty + lineHeightPx(t) + 6 * S), true, &hv);
  float aw = action.empty() ? 0.f : textWidth(t, action);
  drawText(dl, ImVec2(x, ty), t, kMuted, ellipsize(t, note, w - aw - 12 * S));
  if (!action.empty()) {
    drawGradientText(dl, ImVec2(x + w - aw, ty), t, action);
    if (hv) dl->AddLine(ImVec2(x + w - aw, ty + lineHeightPx(t)), ImVec2(x + w, ty + lineHeightPx(t)), gradient(0.5f));
  }
  return clicked;
}

void App::drawDepthPopover(ImVec2 anchor, bool& hovered) {
  float S = scale();
  const std::string& L = lang_;
  const float W = std::round(320 * S), pad = std::round(14 * S), cw = W - 2 * pad;
  const float fieldH = std::round(30 * S), sliderH = std::round(20 * S);
  TextStyle head = ts(Family::Manrope, 600, 10, 0.16f, true);
  TextStyle lab = tk::label(), val = tk::value();
  TextStyle hint = ts(Family::Plex, 400, 11, 0, false, 1.35f);
  float labH = lineHeightPx(lab) + 6 * S;
  bool stereo = depth3d_ && stereoOut_ != StereoOff;
  std::string hintText = stereo ? i18n::tr(L, "stereo.hint") : "";
  float hintH = stereo ? wrappedHeight(hint, hintText, cw) + 10 * S : 0.f;
  std::string footNote;
  float H = std::round(12 * S + lineHeightPx(head) + 12 * S + labH + fieldH + 14 * S + lineHeightPx(lab) + 6 * S +
                       sliderH + hintH + 14 * S + (footNote.empty() ? 0.f : footerHeight()));
  ImVec2 pos = popoverPos(anchor, W, H);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(W, popoverWindowH(pos, H, dockTileTop_)));  // + hover bridge
  ImGui::Begin("##depthpop", nullptr, kPopoverFlags);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  popoverBg(dl, pos, pos + ImVec2(W, H), kPopover, 8 * S);
  float x = pos.x + pad, y = pos.y + 12 * S;
  drawGradientText(dl, ImVec2(x, y), head, std::string(i18n::tr(L, "depth.title")) + " \xC2\xB7 D");
  y += lineHeightPx(head) + 12 * S;
  // Format.
  drawText(dl, ImVec2(x, y), lab, kMuted, i18n::tr(L, "depth.format"));
  y += labH;
  bool comboOpen = false;
  {
    std::vector<Opt> o;
    for (auto& f : depthFormatOptions()) o.push_back({std::to_string(f.first), f.second});
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    std::string out;
    if (selectBox("##depthfmtpop", o, std::to_string(depthFormat()), out, cw, &comboOpen))
      setDepthFormat(std::atoi(out.c_str()));
  }
  y += fieldH + 14 * S;
  // Strength.
  char b[8];
  std::snprintf(b, sizeof b, "%d%%", (int)std::lround(depthStrength_ * 100));
  std::string sl = i18n::tr(L, "depth.strength");
  drawText(dl, ImVec2(x, y), lab, kMuted, ellipsize(lab, sl, cw - textWidth(val, b) - 12 * S));
  drawText(dl, ImVec2(x + cw - textWidth(val, b), y + (lineHeightPx(lab) - lineHeightPx(val)) / 2), val,
           depth3d_ ? kFg : kMuted, b);
  y += lineHeightPx(lab) + 6 * S;
  float v = depthStrength_;
  if (rangeSlider("##depthstr", v, 0.1f, 1.f, 0.05f, ImVec2(x, y), cw, sliderH, depth3d_)) setDepthStrength(v);
  y += sliderH;
  if (stereo) drawWrapped(dl, ImVec2(x, y + 10 * S), cw, hint, kMuted, hintText);
  if (comboOpen) anyComboOpen_ = true;
  hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || ImGui::IsAnyItemActive() || comboOpen;
  ImGui::End();
}

// Hover popover listing choices as selectable cards (name + one-line
// description, radio dot): the 4K upscaler and the TV style. Returns the card
// clicked, -2 for the footer (when given), or -1.
static int choiceCardsPopover(const char* winId, ImVec2 anchor, float tileTop, const std::string& heading,
                              const std::vector<std::pair<std::string, std::string>>& cards, int selected,
                              bool& hovered, const std::string& footNote = "", const std::string& footAction = "") {
  float S = scale();
  TextStyle head = ts(Family::Manrope, 600, 10, 0.16f, true);
  TextStyle name = ts(Family::Plex, 600, 12);
  TextStyle desc = ts(Family::Plex, 400, 11, 0, false, 1.35f);
  float W = 320 * S;
  float innerW = W - 28 * S - 24 * S - 12 * S;
  const int N = (int)cards.size();
  std::vector<float> cardH(N);
  float H = 12 * S + lineHeightPx(head) + 10 * S + 12 * S - 6 * S;
  if (!footNote.empty()) H += footerHeight();
  for (int i = 0; i < N; ++i) {
    cardH[i] = 8 * S + lineHeightPx(name) + 2 * S + wrappedHeight(desc, cards[i].second, innerW) + 8 * S;
    H += cardH[i] + 6 * S;
  }
  ImVec2 pos = popoverPos(anchor, W, H);
  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(W, popoverWindowH(pos, H, tileTop)));
  ImGui::Begin(winId, nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  popoverBg(dl, pos, pos + ImVec2(W, H), kPopover, 8 * S);
  float x = pos.x + 14 * S, y = pos.y + 12 * S;
  drawGradientText(dl, ImVec2(x, y), head, heading);
  y += lineHeightPx(head) + 10 * S;
  int clicked = -1;
  for (int i = 0; i < N; ++i) {
    ImVec2 a(x, y), b(pos.x + W - 14 * S, y + cardH[i]);
    bool sel = selected == i, hv = false;
    std::string id = std::string(winId) + "#" + std::to_string(i);
    if (hit(id.c_str(), a, b, true, &hv)) clicked = i;
    panel(dl, a, b, hv ? ink(0.05f) : 0, sel ? 0 : (hv ? kBorderHover : kBorder), 7 * S);
    if (sel) gradientBorder(dl, a, b, 7 * S, 1.5f * S, 1.f);
    // Radio dot.
    ImVec2 c(a.x + 12 * S, a.y + 8 * S + lineHeightPx(name) / 2);
    dl->AddCircle(c, 5 * S, sel ? gradient(0.5f) : kMuted, 24, 1.3f * S);
    if (sel) dl->AddCircleFilled(c, 2.6f * S, gradient(0.5f), 24);
    float tx = a.x + 24 * S;
    drawText(dl, ImVec2(tx, a.y + 8 * S), name, kFg, ellipsize(name, cards[i].first, b.x - 10 * S - tx));
    drawWrapped(dl, ImVec2(tx, a.y + 8 * S + lineHeightPx(name) + 2 * S), innerW, desc, kMuted, cards[i].second);
    y += cardH[i] + 6 * S;
  }
  if (!footNote.empty()) {
    std::string fid = std::string(winId) + "#foot";
    if (popoverFooter(dl, fid.c_str(), x, y + 2 * S, W - 28 * S, footNote, footAction)) clicked = -2;
  }
  hovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemActive();
  ImGui::End();
  return clicked;
}

// 4K upscaler choice: Off (original resolution) or one of the four
// upscalers. Choosing an upscaler turns upscaling on; Off keeps the last
// upscaler for next time.
void App::drawUpscalePopover(ImVec2 anchor, bool& hovered) {
  static const char* names[kUpscaleModes] = {"upscale.fsr", "upscale.anime4k", "upscale.lanczos", "upscale.nis"};
  static const char* descs[kUpscaleModes] = {"upscale.fsrDesc", "upscale.anime4kDesc", "upscale.lanczosDesc",
                                             "upscale.nisDesc"};
  std::vector<std::pair<std::string, std::string>> cards{{i18n::tr(lang_, "common.off"), i18n::tr(lang_, "upscale.offDesc")}};
  for (int i = 0; i < kUpscaleModes; ++i) cards.push_back({i18n::tr(lang_, names[i]), i18n::tr(lang_, descs[i])});
  int sel = upscale_ ? 1 + std::clamp(upscaleMode_, 0, kUpscaleModes - 1) : 0;
  std::string footNote, footAction;
  int c = choiceCardsPopover("##upscalepop", anchor, dockTileTop_,
                             std::string(i18n::tr(lang_, "upscale.title")) + " \xC2\xB7 U \xC2\xB7 Shift+U", cards, sel,
                             hovered, footNote, footAction);
  if (c == 0) {
    setUpscale(false);
  } else if (c > 0) {
    setUpscaleMode(c - 1);
    setUpscale(true);
  }
}

// Retro look: Off, CRT, VHS or Arcade, drawn over the finished picture.
void App::drawRetroPopover(ImVec2 anchor, bool& hovered) {
  static const char* names[kRetroStyles + 1] = {"common.off", "retro.crt", "retro.vhs", "retro.arcade"};
  static const char* descs[kRetroStyles + 1] = {"retro.offDesc", "retro.crtDesc", "retro.vhsDesc", "retro.arcadeDesc"};
  std::vector<std::pair<std::string, std::string>> cards;
  for (int i = 0; i <= kRetroStyles; ++i) cards.push_back({i18n::tr(lang_, names[i]), i18n::tr(lang_, descs[i])});
  int c = choiceCardsPopover("##retropop", anchor, dockTileTop_, std::string(i18n::tr(lang_, "retro.title")) + " \xC2\xB7 C",
                             cards, retroStyle_, hovered);
  if (c >= 0) setRetroStyle(c);
}

// ---------------------------------------------------------------------------
// About / open-source licences (modal).
void App::drawAbout() {
  float S = scale();
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->Pos);
  ImGui::SetNextWindowSize(vp->Size);
  ImGui::SetNextWindowFocus();
  ImGui::Begin("##aboutback", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(vp->Pos, vp->Pos + vp->Size, withAlpha(kShadow, 0.55f));
  float W = std::min(620 * S, vp->Size.x - 48 * S);
  float H = std::min(vp->Size.y * 0.84f, 720 * S);
  ImVec2 p0(vp->Pos.x + (vp->Size.x - W) / 2, vp->Pos.y + (vp->Size.y - H) / 2), p1(p0.x + W, p0.y + H);
  ImVec2 mouse = ImGui::GetIO().MousePos;
  if (ImGui::IsMouseClicked(0) && !inRect(mouse, p0, p1) && ImGui::IsWindowHovered()) aboutOpen_ = false;
  {
    GlassStyle gs;
    gs.solid = 0.85f;
    gs.shadowAlpha = 0.7f;
    glass(dl, p0, p1, 22 * S, gs);
  }
  // Head: wordmark, version, close.
  float headH = 86 * S;
  {
    TextStyle name = ts(Family::Manrope, 800, 22, 0.0f);
    TextStyle ver = ts(Family::Plex, 500, 12, 0.02f);
    drawGradientText(dl, ImVec2(p0.x + 22 * S, p0.y + 18 * S), name, "UVC Studio");
    drawText(dl, ImVec2(p0.x + 22 * S, p0.y + 18 * S + lineHeightPx(name) + 4 * S), ver, kMuted,
             platform::versionLabel());
    ImVec2 c0(p1.x - 18 * S - 30 * S, p0.y + 18 * S), c1(c0.x + 30 * S, c0.y + 30 * S);
    bool ch;
    if (hit("##aboutclose", c0, c1, true, &ch)) aboutOpen_ = false;
    panel(dl, c0, c1, 0, ch ? kBorderHover : kBorder, 8 * S);
    icons::draw(dl, "close", ImVec2((c0.x + c1.x) / 2, (c0.y + c1.y) / 2), 14 * S, ch ? kFg : kMuted, fbScale_);
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(ImVec2(p0.x, p0.y + headH - 1), ImVec2(p1.x, p0.y + headH), IM_COL32_WHITE);
    shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(p0.x, 0), ImVec2(p1.x, 0), 0.5f);
  }
  // Body.
  const auto& C = components();
  if (licenseOpen_.size() != C.size()) {
    licenseOpen_.assign(C.size(), false);
    licenseText_.assign(C.size(), "");
    licenseLines_.assign(C.size(), {});
    licenseWrapW_.assign(C.size(), -1.f);
  }
  ImGui::SetCursorScreenPos(ImVec2(p0.x + 1, p0.y + headH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * S, 12 * S));
  ImGui::BeginChild("##aboutbody", ImVec2(W - 2, H - headH - 8 * S), ImGuiChildFlags_AlwaysUseWindowPadding);
  ImDrawList* cd = ImGui::GetWindowDrawList();
  float cw = ImGui::GetContentRegionAvail().x;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  TextStyle intro = ts(Family::Plex, 400, 13, 0, false, 1.5f);
  TextStyle secT = ts(Family::Manrope, 600, 10, 0.16f, true);
  TextStyle compT = ts(Family::Manrope, 600, 13);
  TextStyle licT = ts(Family::Plex, 500, 11, 0.02f);
  TextStyle noteT = ts(Family::Plex, 400, 12, 0, false, 1.45f);
  TextStyle textT = ts(Family::Plex, 400, 11, 0, false, 1.45f);
  ImGui::Dummy(ImVec2(cw, 6 * S));
  {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = drawWrapped(cd, p, cw, intro, kMuted, i18n::tr(lang_, "about.privacy"));
    ImGui::Dummy(ImVec2(cw, h + 18 * S));
    p = ImGui::GetCursorScreenPos();
    drawText(cd, p, secT, kFg, i18n::tr(lang_, "about.licenses"));
    ImGui::Dummy(ImVec2(cw, lineHeightPx(secT) + 8 * S));
  }
  for (size_t i = 0; i < C.size(); ++i) {
    ImVec2 q = ImGui::GetCursorScreenPos();
    float rowH = lineHeightPx(compT) + 16 * S;
    bool hv;
    std::string id = "##lic" + std::to_string(i);
    if (hit(id.c_str(), q, ImVec2(q.x + cw, q.y + rowH), true, &hv)) licenseOpen_[i] = !licenseOpen_[i];
    float lw = textWidth(licT, C[i].license);
    drawText(cd, ImVec2(q.x, q.y + 8 * S), compT, hv ? kFg : withAlpha(kFg, 0.92f),
             ellipsize(compT, C[i].name, cw - lw - 40 * S));
    drawText(cd, ImVec2(q.x + cw - 22 * S - lw, q.y + 8 * S + (lineHeightPx(compT) - lineHeightPx(licT)) / 2), licT,
             kMuted, C[i].license);
    TextStyle plus = ts(Family::Plex, 400, 16);
    drawText(cd, ImVec2(q.x + cw - 4 * S - textWidth(plus, "+"), q.y + 6 * S), plus, hv ? kFg : kMuted,
             licenseOpen_[i] ? "−" : "+");
    ImGui::SetCursorScreenPos(q);
    ImGui::Dummy(ImVec2(cw, rowH));
    if (licenseOpen_[i]) {
      if (licenseText_[i].empty()) {
        std::string path =
            platform::joinPath(platform::joinPath(platform::assetsDir(), "licenses"), C[i].file);
        std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
        licenseText_[i] = f ? std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>())
                            : std::string("(license text not found: ") + C[i].file + ")";
      }
      ImVec2 p = ImGui::GetCursorScreenPos();
      float h = drawWrapped(cd, p, cw, noteT, kMuted, C[i].note);
      ImGui::Dummy(ImVec2(cw, h + 8 * S));
      // Only lay out what is on screen (licence texts are long).
      p = ImGui::GetCursorScreenPos();
      ImVec2 clipMin = cd->GetClipRectMin(), clipMax = cd->GetClipRectMax();
      float lh = lineHeightPx(textT);
      float wrapW = cw - 12 * S;
      if (licenseWrapW_[i] != wrapW) {  // wrap once per width, not every frame
        licenseLines_[i].clear();
        for (auto& ln : wrap(textT, licenseText_[i], wrapW)) licenseLines_[i].push_back(ln.text);
        licenseWrapW_[i] = wrapW;
      }
      const auto& lines = licenseLines_[i];
      float boxH = lines.size() * lh + 16 * S;
      cd->AddRectFilled(p, ImVec2(p.x + cw, p.y + boxH), ink(0.04f), 8 * S);
      float ly = p.y + 8 * S;
      for (auto& ln : lines) {
        if (ly + lh >= clipMin.y && ly <= clipMax.y) drawText(cd, ImVec2(p.x + 8 * S, ly), textT, kMuted, ln);
        ly += lh;
      }
      ImGui::Dummy(ImVec2(cw, boxH + 10 * S));
    }
    ImVec2 lp = ImGui::GetCursorScreenPos();
    cd->AddLine(ImVec2(lp.x, lp.y - 0.5f), ImVec2(lp.x + cw, lp.y - 0.5f), kBorder);
  }
  ImGui::Dummy(ImVec2(cw, 16 * S));
  ImGui::PopStyleVar();
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::End();
}


// Tooltip: a glass label with the action name and its shortcut as keycaps,
// above the anchor (below it for top-bar controls).
void App::drawTooltip(ImDrawList* dl) {
  if (tooltip_.empty()) return;
  float S = scale();
  TextStyle t = tk::chip();
  std::vector<std::string> keys = splitKeys(tooltipKey_);
  const float h = std::round(28 * S), padX = 11 * S;
  float kw = keys.empty() ? 0.f : keycaps(dl, ImVec2(0, 0), keys, false);
  float w = std::round(padX + textWidth(t, tooltip_) + (kw > 0 ? 9 * S + kw - 4 * S : 0) + padX);
  ImGuiViewport* vp = ImGui::GetMainViewport();
  float x = std::clamp(tooltipAnchor_.x - w / 2, vp->Pos.x + 8 * S, vp->Pos.x + vp->Size.x - 8 * S - w);
  ImVec2 p0(std::round(x), std::round(tooltipBelow_ ? tooltipAnchor_.y : tooltipAnchor_.y - h));
  ImVec2 p1(p0.x + w, p0.y + h);
  GlassStyle gs;
  gs.solid = 0.78f;
  gs.lens = false;
  gs.shadowAlpha = 0.4f;
  glass(dl, p0, p1, h / 2, gs);
  float cy = (p0.y + p1.y) / 2;
  drawText(dl, ImVec2(p0.x + padX, std::round(cy - lineHeightPx(t) / 2)), t, kFg, tooltip_);
  if (kw > 0) keycaps(dl, ImVec2(p0.x + padX + textWidth(t, tooltip_) + 9 * S, cy), keys, true);
}

// Shortcut sheet (? or H): a centred glass card listing the keys in two
// columns. Any key or click closes it.
void App::drawShortcutSheet(ImDrawList* dl) {
  float S = scale();
  const std::string& L = lang_;
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImVec2 v0 = vp->Pos, v1 = vp->Pos + vp->Size;
  dl->AddRectFilled(v0, v1, withAlpha(kShadow, 0.45f));
  using Keys = std::vector<std::vector<std::string>>;  // alternatives, each a key combination
  struct Row {
    const char* key;
    Keys keys;
  };
  const Row cols[2][8] = {
      {{"session.connect", {{"Enter"}}},
       {"session.disconnect", {{"Ctrl", "D"}}},
       {"dock.snapshot", {{"S"}}},
       {"shortcuts.record", {{"R"}}},
       {"dock.mic", {{"M"}}},
       {"listen.title", {{"L"}}},
       {"overlay.title", {{"W"}}},
       {"picture.mirror", {{"X"}}}},
      {{"upscale.title", {{"U"}}},
       {"upscale.next", {{"Shift", "U"}}},
       {"retro.title", {{"C"}}},
       {"depth.title", {{"D"}}},
       {"settings.title", {{"O"}, {"Ctrl", ","}}},
       {"view.fullscreen", {{"F"}, {"F11"}}},
       {"shortcuts.title", {{"?"}, {"H"}}},
       {"display.interfaceSize", {{"Ctrl", "+", "\xE2\x88\x92", "0"}}}},
  };
  const float W = std::round(std::min(820 * S, vp->Size.x - 48 * S)), pad = std::round(28 * S),
              colGap = std::round(36 * S), rowH = std::round(38 * S);
  const float colW = (W - 2 * pad - colGap) / 2;
  TextStyle title = tk::title(), body = tk::body(), sep = tk::label();
  TextStyle hint = tk::label();
  float H = std::round(pad + lineHeightPx(title) + 16 * S + 8 * rowH + 18 * S + lineHeightPx(hint) + pad - 4 * S);
  ImVec2 p0(std::round(v0.x + (vp->Size.x - W) / 2), std::round(v0.y + (vp->Size.y - H) / 2)), p1(p0.x + W, p0.y + H);
  GlassStyle gs;
  gs.solid = 0.82f;
  gs.shadowAlpha = 0.7f;
  glass(dl, p0, p1, tk::kPanelR * S, gs);
  float y = p0.y + pad;
  gradientIcon(dl, "keyboard", ImVec2(p0.x + pad + 11 * S, y + lineHeightPx(title) / 2), 22 * S, fbScale_);
  drawText(dl, ImVec2(p0.x + pad + 32 * S, y), title, kFg, i18n::tr(L, "shortcuts.title"));
  y += lineHeightPx(title) + 16 * S;
  for (int c = 0; c < 2; ++c) {
    float cx = p0.x + pad + c * (colW + colGap);
    for (int i = 0; i < 8; ++i) {
      const Row& r = cols[c][i];
      float ry = y + i * rowH, cy = ry + rowH / 2;
      if (i) dl->AddLine(ImVec2(cx, ry), ImVec2(cx + colW, ry), kBorder, 1.f);
      // Keys, right-aligned; alternatives separated by a slash.
      float kw = 0;
      const float slashW = textWidth(sep, "/") + 12 * S;
      for (size_t k = 0; k < r.keys.size(); ++k) kw += keycaps(dl, ImVec2(0, 0), r.keys[k], false) + (k ? slashW : 0);
      float kx = cx + colW - kw;
      for (size_t k = 0; k < r.keys.size(); ++k) {
        if (k) {
          drawText(dl, ImVec2(kx + 6 * S, std::round(cy - lineHeightPx(sep) / 2)), sep, kMuted, "/");
          kx += slashW;
        }
        kx += keycaps(dl, ImVec2(kx, cy), r.keys[k], true);
      }
      std::string lab = ellipsize(body, i18n::tr(L, r.key), colW - kw - 16 * S);
      drawText(dl, ImVec2(cx, std::round(cy - lineHeightPx(body) / 2)), body, kFg, lab);
    }
  }
  y += 8 * rowH + 18 * S;
  std::string h = i18n::tr(L, "shortcuts.hint");
  drawText(dl, ImVec2(std::round((p0.x + p1.x) / 2 - textWidth(hint, h) / 2), y), hint, kMuted, h);
}

void App::drawToasts(ImDrawList* dl) {
  float S = scale();
  std::lock_guard<std::mutex> lk(toastMu_);
  double now = nowSeconds();
  toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(), [&](const Toast& t) { return t.until < now; }),
                toasts_.end());
  // Stacked at the top right of the stage.
  float y = stageMin_.y + 16 * S;
  TextStyle t = ts(Family::Plex, 500, 11, 0.04f);
  TextStyle tb = ts(Family::Plex, 600, 11, 0.04f);
  ImVec2 mouse = ImGui::GetIO().MousePos;
  for (int i = (int)toasts_.size() - 1; i >= 0; --i) {
    auto& T = toasts_[i];
    std::string lead;
    std::string tail = T.path.empty() ? "" : std::string("  ") + i18n::tr(lang_, "toast.show");
    float w = 12 * S + 14 * S + 8 * S + textWidth(t, lead) + textWidth(tb, T.text) + textWidth(t, tail) + 12 * S;
    float h = lineHeightPx(t) + 12 * S + 2;
    ImVec2 p0(stageMax_.x - 16 * S - w, y), p1(stageMax_.x - 16 * S, y + h);
    float fade = (float)std::min(1.0, (T.until - now) / 0.4);
    bool hov = inRect(mouse, p0, p1);
    {
      int v0 = dl->VtxBuffer.Size;
      GlassStyle gs;
      gs.solid = 0.7f;
      gs.shadowAlpha = 0.45f;
      if (T.error) {
        gs.tint = kRed;
        gs.tintAmount = 0.14f;
      }
      glass(dl, p0, p1, h / 2, gs);
      if (fade < 1.f) alphaRamp(dl, v0, dl->VtxBuffer.Size, 0, 1, fade, fade);
    }
    float x = p0.x + 12 * S;
    icons::draw(dl, T.error ? "close" : "check", ImVec2(x + 7 * S, (p0.y + p1.y) / 2), 14 * S,
                withAlpha(T.error ? kRed : kGreen, fade), fbScale_);
    x += 14 * S + 8 * S;
    float ty = p0.y + 6 * S + 1;
    drawText(dl, ImVec2(x, ty), t, withAlpha(kMuted, fade), lead);
    x += textWidth(t, lead);
    drawText(dl, ImVec2(x, ty), tb, withAlpha(T.error ? kRed : kFg, fade), T.text);
    x += textWidth(tb, T.text);
    if (!tail.empty()) drawText(dl, ImVec2(x, ty), t, withAlpha(hov ? kFg : kBlue, fade), tail);
    if (hov && !T.path.empty()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      if (ImGui::IsMouseClicked(0)) {
        platform::revealInFolder(T.path);
        T.until = now + 0.4;
      }
    }
    y += h + 8 * S;
  }
}
