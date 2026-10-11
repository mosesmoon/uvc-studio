#include "Ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

#include "Platform.h"
#include "imgui_internal.h"

namespace ui {

ImU32 rgba(int r, int g, int b, float a) { return IM_COL32(r, g, b, (int)std::lround(a * 255.f)); }
ImU32 withAlpha(ImU32 c, float a) {
  int al = (int)((c >> IM_COL32_A_SHIFT) & 0xff);
  al = (int)std::lround(al * a);
  return (c & ~IM_COL32_A_MASK) | ((ImU32)al << IM_COL32_A_SHIFT);
}

ImU32 mix(ImU32 a, ImU32 b, float t) {
  auto ch = [&](int sh) {
    float x = (float)((a >> sh) & 0xff), y = (float)((b >> sh) & 0xff);
    return (ImU32)std::lround(x + (y - x) * t) << sh;
  };
  return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | ch(IM_COL32_A_SHIFT);
}

namespace {
struct Palette {
  ImU32 red, green, blue, purple, bg, panel, panel2, fg, muted, border, borderHover, stage, popover, settingsBg, ink,
      shadow;
  ImU32 grad[4];
};
// Surface tokens (design spec §9.1): background, panel, raised, text,
// secondary text, hairline, stage. The brand colours are the same in both
// themes, deepened slightly in the light one for contrast.
const Palette kDark = {
    IM_COL32(0xFF, 0x4D, 0x6D, 255), IM_COL32(0x27, 0xF5, 0x9A, 255), IM_COL32(0x12, 0xB9, 0xFF, 255),
    IM_COL32(0xA8, 0x55, 0xF7, 255),
    IM_COL32(0x0F, 0x11, 0x15, 255),  // background
    IM_COL32(0x16, 0x1A, 0x21, 255),  // panel
    IM_COL32(0x1D, 0x22, 0x2B, 255),  // raised
    IM_COL32(0xEC, 0xEF, 0xF4, 255),  // text
    IM_COL32(0xEC, 0xEF, 0xF4, 158),  // secondary text (62 %)
    IM_COL32(0xFF, 0xFF, 0xFF, 26),   // hairline (10 %)
    IM_COL32(0xFF, 0xFF, 0xFF, 51),   // hairline, hovered (20 %)
    IM_COL32(0x07, 0x08, 0x0A, 255),  // stage
    IM_COL32(0x1D, 0x22, 0x2B, 255),  // popover (raised)
    IM_COL32(0x16, 0x1A, 0x21, 255),  // settings (panel)
    IM_COL32(0xFF, 0xFF, 0xFF, 255),  // ink
    IM_COL32(0x00, 0x00, 0x00, 255),  // shadow
    {IM_COL32(0x27, 0xF5, 0x9A, 255), IM_COL32(0x12, 0xB9, 0xFF, 255), IM_COL32(0xA8, 0x55, 0xF7, 255),
     IM_COL32(0xFF, 0x4D, 0x6D, 255)}};
const Palette kLight = {
    IM_COL32(0xE0, 0x2F, 0x52, 255), IM_COL32(0x0A, 0xA3, 0x66, 255), IM_COL32(0x08, 0x8C, 0xD6, 255),
    IM_COL32(0x86, 0x3B, 0xE0, 255),
    IM_COL32(0xF3, 0xF4, 0xF7, 255),  // background
    IM_COL32(0xFF, 0xFF, 0xFF, 255),  // panel
    IM_COL32(0xEE, 0xF0, 0xF4, 255),  // raised
    IM_COL32(0x14, 0x16, 0x1B, 255),  // text
    IM_COL32(0x14, 0x16, 0x1B, 153),  // secondary text (60 %)
    IM_COL32(0x00, 0x00, 0x00, 23),   // hairline (9 %)
    IM_COL32(0x00, 0x00, 0x00, 46),   // hairline, hovered (18 %)
    IM_COL32(0xE6, 0xE8, 0xEC, 255),  // stage
    IM_COL32(0xFF, 0xFF, 0xFF, 255),  // popover (panel)
    IM_COL32(0xFF, 0xFF, 0xFF, 255),  // settings (panel)
    IM_COL32(0x14, 0x16, 0x1B, 255),  // ink
    IM_COL32(0x2A, 0x2F, 0x45, 255),  // shadow
    {IM_COL32(0x0B, 0xC4, 0x7C, 255), IM_COL32(0x08, 0x9C, 0xEC, 255), IM_COL32(0x93, 0x42, 0xEE, 255),
     IM_COL32(0xEC, 0x37, 0x5A, 255)}};
float gLight = 0.f;
ImU32 gGrad[4];
}  // namespace

ImU32 kRed, kGreen, kBlue, kPurple, kBg, kPanel, kPanel2, kFg, kMuted, kBorder, kBorderHover, kStage, kPopover,
    kSettingsBg, kInk, kShadow;

void setTheme(float t) {
  gLight = std::clamp(t, 0.f, 1.f);
  const Palette &a = kDark, &b = kLight;
  auto m = [&](ImU32 x, ImU32 y) { return mix(x, y, gLight); };
  kRed = m(a.red, b.red);
  kGreen = m(a.green, b.green);
  kBlue = m(a.blue, b.blue);
  kPurple = m(a.purple, b.purple);
  kBg = m(a.bg, b.bg);
  kPanel = m(a.panel, b.panel);
  kPanel2 = m(a.panel2, b.panel2);
  kFg = m(a.fg, b.fg);
  kMuted = m(a.muted, b.muted);
  kBorder = m(a.border, b.border);
  kBorderHover = m(a.borderHover, b.borderHover);
  kStage = m(a.stage, b.stage);
  kPopover = m(a.popover, b.popover);
  kSettingsBg = m(a.settingsBg, b.settingsBg);
  kInk = m(a.ink, b.ink);
  kShadow = m(a.shadow, b.shadow);
  for (int i = 0; i < 4; ++i) gGrad[i] = m(a.grad[i], b.grad[i]);
}
float themeLight() { return gLight; }
static const bool gThemeInit = (setTheme(0.f), true);

ImU32 ink(float alpha) { return withAlpha(kInk, alpha); }

ImU32 gradient(float t, float alpha) {
  // Stops: green 0, blue .36, purple .70, red 1.
  static const float stops[4] = {0.f, 0.36f, 0.70f, 1.f};
  t = std::clamp(t, 0.f, 1.f);
  int i = 0;
  while (i < 2 && t > stops[i + 1]) ++i;
  float f = (t - stops[i]) / (stops[i + 1] - stops[i]);
  // Smoothstep between stops keeps the bands soft.
  f = f * f * (3.f - 2.f * f);
  return withAlpha(mix(gGrad[i], gGrad[i + 1], f), alpha);
}

void shadeGradient(ImDrawList* dl, int v0, int v1, ImVec2 p0, ImVec2 p1, float alphaMul, float t0, float t1) {
  ImVec2 d(p1.x - p0.x, p1.y - p0.y);
  float len2 = d.x * d.x + d.y * d.y;
  if (len2 <= 0) len2 = 1;
  for (int i = v0; i < v1 && i < dl->VtxBuffer.Size; ++i) {
    ImDrawVert& v = dl->VtxBuffer[i];
    float t = ((v.pos.x - p0.x) * d.x + (v.pos.y - p0.y) * d.y) / len2;
    t = t0 + (t1 - t0) * std::clamp(t, 0.f, 1.f);
    float a = (float)((v.col >> IM_COL32_A_SHIFT) & 0xff) / 255.f * alphaMul;
    v.col = gradient(t, a);
  }
}

void gradientRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float alpha) {
  int v0 = dl->VtxBuffer.Size;
  dl->AddRectFilled(a, b, IM_COL32_WHITE, r);
  shadeGradient(dl, v0, dl->VtxBuffer.Size, a, b, alpha);
}

void gradientBorder(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float th, float alpha) {
  int v0 = dl->VtxBuffer.Size;
  dl->AddRect(ImVec2(a.x + th * 0.5f, a.y + th * 0.5f), ImVec2(b.x - th * 0.5f, b.y - th * 0.5f), IM_COL32_WHITE, r,
              0, th);
  shadeGradient(dl, v0, dl->VtxBuffer.Size, a, b, alpha);
}

void radialGlow(ImDrawList* dl, ImVec2 c, float r, ImU32 col, int n) {
  // Triangle fan: opaque centre, transparent rim, plus a mid ring so the
  // falloff is closer to a gaussian than a cone.
  const ImVec2 uv = dl->_Data->TexUvWhitePixel;
  ImU32 mid = withAlpha(col, 0.35f), rim = withAlpha(col, 0.f);
  dl->PrimReserve(n * 9, n * 2 + 1);
  ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
  dl->PrimWriteVtx(c, uv, col);
  for (int i = 0; i < n; ++i) {
    float a = (float)i / n * 6.2831853f;
    ImVec2 dir(std::cos(a), std::sin(a));
    dl->PrimWriteVtx(ImVec2(c.x + dir.x * r * 0.45f, c.y + dir.y * r * 0.45f), uv, mid);
    dl->PrimWriteVtx(ImVec2(c.x + dir.x * r, c.y + dir.y * r), uv, rim);
  }
  for (int i = 0; i < n; ++i) {
    int j = (i + 1) % n;
    ImDrawIdx mi = base + 1 + i * 2, mj = base + 1 + j * 2;
    dl->PrimWriteIdx(base);
    dl->PrimWriteIdx(mi);
    dl->PrimWriteIdx(mj);
    dl->PrimWriteIdx(mi);
    dl->PrimWriteIdx(mi + 1);
    dl->PrimWriteIdx(mj + 1);
    dl->PrimWriteIdx(mi);
    dl->PrimWriteIdx(mj + 1);
    dl->PrimWriteIdx(mj);
  }
}

static float gScale = 1.f;
float scale() { return gScale; }
void setScale(float s) { gScale = s; }

// ---------------------------------------------------------------------------
// Fonts: Manrope (display) and IBM Plex Sans (body), with IBM Plex Sans Thai
// and Noto Sans CJK TC merged in for Thai and CJK text.
static ImFont* gManrope[5] = {};  // 400, 500, 600, 700, 800
static ImFont* gPlex[3] = {};     // 400, 500, 600
static std::vector<char> gCjkRegular, gCjkBold;
static std::vector<char> gThai[3];  // IBM Plex Sans Thai 400, 500, 600

// Codepoints taken from Manrope / IBM Plex Sans: Latin, Latin-1, Latin
// Extended-A/B, combining marks and Latin Extended Additional (the whole
// Vietnamese alphabet), punctuation, currency, arrows, maths and the small
// geometric shapes. Everything else falls through to the merged fonts.
static const ImWchar kLatinRanges[] = {0x0020, 0x024F, 0x0300, 0x036F, 0x1E00, 0x1EFF, 0x2000, 0x206F,
                                       0x20A0, 0x20CF, 0x2100, 0x214F, 0x2190, 0x21FF, 0x2200, 0x22FF,
                                       0x25A0, 0x25FF, 0};
// Thai block (consonants, vowels, tone marks, digits).
static const ImWchar kThaiRanges[] = {0x0E00, 0x0E7F, 0};

static bool readFile(const std::string& p, std::vector<char>& out) {
  std::ifstream f(std::filesystem::u8path(p), std::ios::binary);
  if (!f) return false;
  out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  return !out.empty();
}

// Line span (ascent − descent from 'hhea') in em units of a TrueType /
// OpenType file; 0 when it cannot be read.
static float lineSpanEm(const std::vector<char>& f) {
  auto u16 = [&](size_t o) { return o + 2 <= f.size() ? (unsigned)((unsigned char)f[o] << 8 | (unsigned char)f[o + 1]) : 0u; };
  auto u32 = [&](size_t o) { return (u16(o) << 16) | u16(o + 2); };
  unsigned n = u16(4), upem = 0;
  int asc = 0, desc = 0;
  for (unsigned i = 0; i < n; ++i) {
    size_t rec = 12 + 16 * (size_t)i;
    if (rec + 16 > f.size()) break;
    std::string tag(f.data() + rec, 4);
    size_t off = u32(rec + 8);
    if (tag == "head") upem = u16(off + 18);
    if (tag == "hhea") {
      asc = (int16_t)u16(off + 4);
      desc = (int16_t)u16(off + 6);
    }
  }
  return upem ? (float)(asc - desc) / upem : 0.f;
}

bool loadFonts(const std::string& dir) {
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->Clear();
  std::string fd = platform::joinPath(dir, "fonts");
  bool haveCjk = readFile(platform::joinPath(fd, "NotoSansCJKtc-Regular.otf"), gCjkRegular);
  haveCjk &= readFile(platform::joinPath(fd, "NotoSansCJKtc-Bold.otf"), gCjkBold);
  const int thaiW[3] = {400, 500, 600};
  for (int i = 0; i < 3; ++i)
    readFile(platform::joinPath(fd, "IBMPlexSansThai-" + std::to_string(thaiW[i]) + ".ttf"), gThai[i]);
  // `scale` sizes the merged glyphs relative to the main font.
  auto merge = [&](std::vector<char>& data, const ImWchar* ranges, float scale = 1.f) {
    if (data.empty()) return;
    ImFontConfig m;
    m.SizePixels = 14.f * scale;
    m.MergeMode = true;
    m.FontDataOwnedByAtlas = false;
    m.OversampleH = 1;
    m.OversampleV = 1;
    m.GlyphRanges = ranges;
    io.Fonts->AddFontFromMemoryTTF(data.data(), (int)data.size(), m.SizePixels, &m);
  };
  auto add = [&](const std::string& file, int weight) -> ImFont* {
    std::vector<char> data;
    ImFont* f = nullptr;
    float span = 0.f;
    if (readFile(platform::joinPath(fd, file), data)) {
      span = lineSpanEm(data);
      void* mem = IM_ALLOC(data.size());
      memcpy(mem, data.data(), data.size());
      ImFontConfig c;
      c.FontDataOwnedByAtlas = true;
      c.OversampleH = 2;
      c.OversampleV = 1;
      c.PixelSnapH = false;
      c.GlyphRanges = kLatinRanges;
      f = io.Fonts->AddFontFromMemoryTTF(mem, (int)data.size(), 14.f, &c);
    } else {
      f = io.Fonts->AddFontDefault();
    }
    // Thai in the nearest available weight (the Thai family stops at 600).
    // Glyphs are sized by each font's line span, and IBM Plex Sans Thai has
    // a much taller one (room for stacked marks): scale it up so its letters
    // match the main font's em.
    std::vector<char>& thai = gThai[weight <= 400 ? 0 : weight <= 500 ? 1 : 2];
    float thaiSpan = thai.empty() ? 0.f : lineSpanEm(thai);
    merge(thai, kThaiRanges, span > 0 && thaiSpan > 0 ? thaiSpan / span : 1.f);
    if (haveCjk) merge(weight >= 600 ? gCjkBold : gCjkRegular, nullptr);
    return f;
  };
  const int mw[5] = {400, 500, 600, 700, 800};
  for (int i = 0; i < 5; ++i) gManrope[i] = add("Manrope-" + std::to_string(mw[i]) + ".ttf", mw[i]);
  const int pw[3] = {400, 500, 600};
  for (int i = 0; i < 3; ++i) gPlex[i] = add("IBMPlexSans-" + std::to_string(pw[i]) + ".ttf", pw[i]);
  io.FontDefault = gPlex[1];
  return haveCjk;
}

ImFont* font(Family f, int w) {
  if (f == Family::Manrope) {
    int i = w <= 400 ? 0 : w <= 500 ? 1 : w <= 600 ? 2 : w <= 700 ? 3 : 4;
    return gManrope[i];
  }
  int i = w <= 400 ? 0 : w <= 500 ? 1 : 2;
  return gPlex[i];
}

TextStyle ts(Family fam, int weight, float size, float tr, bool up, float lh) {
  TextStyle s;
  s.f = font(fam, weight);
  s.size = size * gScale;
  s.tracking = tr;
  s.upper = up;
  s.lineHeight = lh;
  return s;
}

// ---------------------------------------------------------------------------
static unsigned decode(const char*& p, const char* end) {
  unsigned c = 0;
  int n = ImTextCharFromUtf8(&c, p, end);
  p += n > 0 ? n : 1;
  return c;
}

static void encode(std::string& out, unsigned c) {
  char buf[5];
  int n = ImTextCharToUtf8(buf, c);
  out.append(buf, n);
}

std::string upper(const std::string& s) {
  std::string out;
  const char* p = s.c_str();
  const char* e = p + s.size();
  while (p < e) {
    unsigned c = decode(p, e);
    if (c >= 'a' && c <= 'z')
      c -= 32;
    else if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
      c -= 32;
    encode(out, c);
  }
  return out;
}

struct FontScope {
  explicit FontScope(const TextStyle& s) { ImGui::PushFont(s.f, s.size); }
  ~FontScope() { ImGui::PopFont(); }
};

static float advanceOf(ImFontBaked* b, unsigned c) { return b->GetCharAdvance((ImWchar)c); }

static float widthRaw(const TextStyle& s, const std::string& t) {
  FontScope fs(s);
  ImFontBaked* b = ImGui::GetFontBaked();
  float w = 0;
  int n = 0;
  const char* p = t.c_str();
  const char* e = p + t.size();
  while (p < e) {
    unsigned c = decode(p, e);
    if (c == '\n') continue;
    w += advanceOf(b, c);
    ++n;
  }
  return w + n * s.tracking * s.size;
}

float textWidth(const TextStyle& s, const std::string& text) {
  return widthRaw(s, s.upper ? upper(text) : text);
}

float lineHeightPx(const TextStyle& s) {
  if (s.lineHeight > 0) return s.lineHeight * s.size;
  // Natural line height of Manrope / IBM Plex Sans: ~1.3 em.
  return s.size * 1.3f;
}

void drawText(ImDrawList* dl, ImVec2 pos, const TextStyle& s, ImU32 col, const std::string& text0) {
  const std::string text = s.upper ? upper(text0) : text0;
  FontScope fs(s);
  ImFontBaked* b = ImGui::GetFontBaked();
  float content = b->Ascent - b->Descent;
  float lh = lineHeightPx(s);
  float y = pos.y + (lh - content) * 0.5f;
  y = std::floor(y * 1.f + 0.5f);
  if (s.tracking == 0.f) {
    dl->AddText(s.f, s.size, ImVec2(pos.x, y), col, text.c_str(), text.c_str() + text.size());
    return;
  }
  float x = pos.x;
  const char* p = text.c_str();
  const char* e = p + text.size();
  float tr = s.tracking * s.size;
  while (p < e) {
    unsigned c = decode(p, e);
    s.f->RenderChar(dl, s.size, ImVec2(x, y), col, (ImWchar)c);
    x += advanceOf(b, c) + tr;
  }
}

std::string ellipsize(const TextStyle& s, const std::string& text, float maxW) {
  if (textWidth(s, text) <= maxW) return text;
  const std::string ell = "…";
  float ew = textWidth(s, ell);
  std::string out;
  const char* p = text.c_str();
  const char* e = p + text.size();
  float w = 0;
  FontScope fs(s);
  ImFontBaked* b = ImGui::GetFontBaked();
  while (p < e) {
    const char* start = p;
    unsigned c = decode(p, e);
    float a = advanceOf(b, c) + s.tracking * s.size;
    if (w + a + ew > maxW) break;
    w += a;
    out.append(start, p - start);
  }
  return out + ell;
}

static bool isCjk(unsigned c) {
  return (c >= 0x2E80 && c <= 0x9FFF) || (c >= 0xF900 && c <= 0xFAFF) ||
         (c >= 0xFF00 && c <= 0xFFEF) || (c >= 0x3000 && c <= 0x303F);
}
static bool noLineStart(unsigned c) {
  static const unsigned list[] = {0x3001, 0x3002, 0xFF0C, 0xFF0E, 0xFF1A, 0xFF1B, 0xFF01, 0xFF1F, 0xFF09, 0x300D,
                                  0x300F, 0x3011, 0x300B, 0x3009, 0x30FC, 0x2026, ',', '.', '!', '?', ':', ';',
                                  ')', 0x00B7, 0x2014};
  for (unsigned x : list)
    if (c == x) return true;
  return false;
}

// Splits text into breakable units.
struct Unit {
  std::string text;
  bool space = false;
  bool newline = false;
  bool cjk = false;
  unsigned first = 0;
};
static std::vector<Unit> units(const std::string& t) {
  std::vector<Unit> out;
  const char* p = t.c_str();
  const char* e = p + t.size();
  Unit cur;
  auto flush = [&] {
    if (!cur.text.empty()) out.push_back(cur);
    cur = Unit{};
  };
  while (p < e) {
    const char* s = p;
    unsigned c = decode(p, e);
    if (c == '\n') {
      flush();
      Unit nl;
      nl.newline = true;
      out.push_back(nl);
      continue;
    }
    if (c == ' ' || c == '\t') {
      flush();
      Unit sp;
      sp.text = " ";
      sp.space = true;
      sp.first = ' ';
      out.push_back(sp);
      continue;
    }
    if (isCjk(c)) {
      flush();
      Unit u;
      u.text.assign(s, p - s);
      u.cjk = true;
      u.first = c;
      out.push_back(u);
      continue;
    }
    if (cur.text.empty()) cur.first = c;
    cur.text.append(s, p - s);
    // Allow a break after hyphens / slashes / em dashes inside long words.
    if (c == '-' || c == '/' || c == 0x2014) flush();
  }
  flush();
  return out;
}

std::vector<Line> wrap(const TextStyle& s, const std::string& text0, float maxW) {
  const std::string text = s.upper ? upper(text0) : text0;
  TextStyle raw = s;
  raw.upper = false;
  std::vector<Line> lines;
  Line cur;
  auto us = units(text);
  auto push = [&] {
    // trim trailing spaces
    while (!cur.text.empty() && cur.text.back() == ' ') {
      cur.text.pop_back();
    }
    cur.width = widthRaw(raw, cur.text);
    lines.push_back(cur);
    cur = Line{};
  };
  for (size_t i = 0; i < us.size(); ++i) {
    const Unit& u = us[i];
    if (u.newline) {
      push();
      continue;
    }
    if (u.space && cur.text.empty()) continue;
    float w = widthRaw(raw, cur.text + u.text);
    if (w > maxW && !cur.text.empty() && !u.space && !noLineStart(u.first)) {
      push();
      if (u.space) continue;
    }
    cur.text += u.text;
  }
  if (!cur.text.empty() || lines.empty()) push();
  return lines;
}

float wrappedHeight(const TextStyle& s, const std::string& text, float maxW) {
  return wrap(s, text, maxW).size() * lineHeightPx(s);
}

float drawWrapped(ImDrawList* dl, ImVec2 pos, float maxW, const TextStyle& s, ImU32 col, const std::string& text,
                  int align) {
  auto lines = wrap(s, text, maxW);
  TextStyle raw = s;
  raw.upper = false;
  float lh = lineHeightPx(s);
  float y = pos.y;
  for (auto& l : lines) {
    float x = pos.x;
    if (align == 1) x = pos.x + (maxW - l.width) * 0.5f;
    drawText(dl, ImVec2(x, y), raw, col, l.text);
    y += lh;
  }
  return y - pos.y;
}

void drawGradientText(ImDrawList* dl, ImVec2 pos, const TextStyle& s, const std::string& text, float alpha) {
  int v0 = dl->VtxBuffer.Size;
  drawText(dl, pos, s, IM_COL32_WHITE, text);
  float w = textWidth(s, text);
  shadeGradient(dl, v0, dl->VtxBuffer.Size, pos, ImVec2(pos.x + w, pos.y + lineHeightPx(s) * 0.35f), alpha);
}

float drawGradientWrapped(ImDrawList* dl, ImVec2 pos, float maxW, const TextStyle& s, const std::string& text,
                          int align) {
  int v0 = dl->VtxBuffer.Size;
  float h = drawWrapped(dl, pos, maxW, s, IM_COL32_WHITE, text, align);
  // Gradient spans the widest line, diagonally down the block.
  float w = 0;
  for (auto& l : wrap(s, text, maxW)) w = std::max(w, l.width);
  float x0 = align == 1 ? pos.x + (maxW - w) / 2 : pos.x;
  shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(x0, pos.y), ImVec2(x0 + w, pos.y + h * 0.6f));
  return h;
}

void panel(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 fill, ImU32 border, float r, float bw) {
  if (fill & IM_COL32_A_MASK) dl->AddRectFilled(a, b, fill, r);
  if (border & IM_COL32_A_MASK) dl->AddRect(ImVec2(a.x + 0.5f, a.y + 0.5f), ImVec2(b.x - 0.5f, b.y - 0.5f), border, r, 0, bw);
}

void shadow(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float spread, float alpha) {
  const int steps = 8;
  for (int i = steps; i >= 1; --i) {
    float t = (float)i / steps;
    float g = spread * t;
    float al = alpha * (1.f - t) * (1.f - t) * 0.35f;
    // Lighter, tinted shadows in the light theme.
    al *= 1.f - 0.55f * gLight;
    dl->AddRectFilled(ImVec2(a.x - g * 0.5f, a.y - g * 0.2f), ImVec2(b.x + g * 0.5f, b.y + g), withAlpha(kShadow, al),
                      r + g * 0.5f);
  }
}


// ---------------------------------------------------------------------------
namespace {
ImTextureID gBackdrop = 0;
bool gBackdropSuspended = false;
ImVec2 gBdA, gBdB;
}  // namespace

void suspendBackdrop(bool on) { gBackdropSuspended = on; }

void setBackdrop(ImTextureID tex, ImVec2 a, ImVec2 b) {
  gBackdrop = tex;
  gBdA = a;
  gBdB = b;
}

void alphaRamp(ImDrawList* dl, int v0, int v1, float y0, float y1, float a0, float a1) {
  float span = y1 - y0;
  for (int i = v0; i < v1; ++i) {
    ImDrawVert& v = dl->VtxBuffer[i];
    float t = span != 0 ? std::clamp((v.pos.y - y0) / span, 0.f, 1.f) : 0.f;
    float m = a0 + (a1 - a0) * t;
    unsigned a = (v.col >> IM_COL32_A_SHIFT) & 0xFF;
    a = (unsigned)std::clamp(a * m, 0.f, 255.f);
    v.col = (v.col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
  }
}

void glass(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, const GlassStyle& st) {
  float S = scale();
  float L = gLight;
  if (st.shadowAlpha > 0) shadow(dl, a, b, r, 26 * S, st.shadowAlpha);
  // Backdrop: the blurred picture behind, slightly magnified like a lens.
  if (gBackdrop && !gBackdropSuspended && gBdB.x > gBdA.x && gBdB.y > gBdA.y) {
    float W = gBdB.x - gBdA.x, H = gBdB.y - gBdA.y;
    ImVec2 c((a.x + b.x) / 2, (a.y + b.y) / 2);
    float k = st.lens ? 0.9f : 1.f;
    ImVec2 qa(c.x + (a.x - c.x) * k, c.y + (a.y - c.y) * k), qb(c.x + (b.x - c.x) * k, c.y + (b.y - c.y) * k);
    ImVec2 uva((qa.x - gBdA.x) / W, (qa.y - gBdA.y) / H), uvb((qb.x - gBdA.x) / W, (qb.y - gBdA.y) / H);
    dl->AddImageRounded(ImTextureRef(gBackdrop), a, b, uva, uvb, IM_COL32(255, 255, 255, 255), r);
  }
  // Body: smoky glass in the dark theme, milky glass in the light one.
  ImU32 base = mix(IM_COL32(26, 27, 34, 255), IM_COL32(255, 255, 255, 255), L);
  float fillA = (0.38f + 0.18f * L) + st.solid * (0.5f - 0.1f * L);
  dl->AddRectFilled(a, b, withAlpha(base, std::min(0.94f, fillA)), r);
  if (st.gradientTint) {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, (int)(255 * (st.tintAmount > 0 ? st.tintAmount : 0.85f))), r);
    shadeGradient(dl, v0, dl->VtxBuffer.Size, ImVec2(a.x, a.y), ImVec2(b.x, b.y), 1.f);
  } else if (st.tint & IM_COL32_A_MASK) {
    dl->AddRectFilled(a, b, withAlpha(st.tint, st.tintAmount), r);
  }
  // Sheen: light falling on the top half.
  {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 255), r);
    alphaRamp(dl, v0, dl->VtxBuffer.Size, a.y, a.y + (b.y - a.y) * 0.55f, 0.16f + 0.10f * L, 0.f);
  }
  // Rim: bright on top, fading towards the bottom; a faint outer line keeps
  // light glass visible on light backgrounds.
  if (L > 0.01f) dl->AddRect(ImVec2(a.x - 0.5f, a.y - 0.5f), ImVec2(b.x + 0.5f, b.y + 0.5f), IM_COL32(20, 22, 30, (int)(28 * L)), r + 0.5f, 0, 1.f);
  {
    int v0 = dl->VtxBuffer.Size;
    dl->AddRect(ImVec2(a.x + 0.5f, a.y + 0.5f), ImVec2(b.x - 0.5f, b.y - 0.5f), IM_COL32(255, 255, 255, 255), r, 0,
                std::max(1.f, 1.1f * S));
    alphaRamp(dl, v0, dl->VtxBuffer.Size, a.y, b.y, 0.62f, 0.14f);
  }
}

}  // namespace ui
