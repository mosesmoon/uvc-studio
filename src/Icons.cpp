// Icons (24×24), rasterised with nanosvg into white alpha textures and tinted
// at draw time. All artwork is UVC Studio's own, in two sets:
//  - "glass": solid, rounded glyphs for the dock / toolbar (fills with even-odd
//    cut-outs, 2 px rounded strokes);
//  - "line": friendly geometric line icons (1.75 px, round caps and joins) for
//    everything else, and as the line variant of every dock glyph.
#include "Icons.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#include "gl.h"

namespace {

// ---- glass set helpers ----
const char* S2 = "fill=\"none\" stroke=\"#fff\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"";
std::string fillEO(const std::string& d) { return "<path d=\"" + d + "\" fill=\"#fff\" fill-rule=\"evenodd\"/>"; }
std::string stroke2(const std::string& d) { return "<path d=\"" + d + "\" " + S2 + "/>"; }
std::string num(float v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.2f", v);
  return b;
}
// Closed circle as path data (for even-odd holes).
std::string circ(float cx, float cy, float r) {
  return "M" + num(cx + r) + " " + num(cy) + "a" + num(r) + " " + num(r) + " 0 1 1 " + num(-2 * r) + " 0a" + num(r) +
         " " + num(r) + " 0 1 1 " + num(2 * r) + " 0z";
}
// Rounded rectangle as path data.
std::string rrect(float x, float y, float w, float h, float r) {
  return "M" + num(x + r) + " " + num(y) + "h" + num(w - 2 * r) + "a" + num(r) + " " + num(r) + " 0 0 1 " + num(r) +
         " " + num(r) + "v" + num(h - 2 * r) + "a" + num(r) + " " + num(r) + " 0 0 1 " + num(-r) + " " + num(r) + "h" +
         num(-(w - 2 * r)) + "a" + num(r) + " " + num(r) + " 0 0 1 " + num(-r) + " " + num(-r) + "v" +
         num(-(h - 2 * r)) + "a" + num(r) + " " + num(r) + " 0 0 1 " + num(r) + " " + num(-r) + "z";
}
// Gear: 8 rounded-off teeth around a hub with a hole.
std::string gear() {
  std::string d;
  const int teeth = 8;
  const float ro = 10.6f, ri = 8.2f, cx = 12, cy = 12;
  for (int i = 0; i < teeth * 4; ++i) {
    float a = (float)i / (teeth * 4) * 6.2831853f - 1.5707963f;
    int ph = i % 4;
    float rr = (ph == 1 || ph == 2) ? ro : ri;
    d += (i == 0 ? "M" : "L") + num(cx + rr * std::cos(a)) + " " + num(cy + rr * std::sin(a));
  }
  d += "z" + circ(cx, cy, 3.4f);
  return fillEO(d);
}

std::string glassBody(const std::string& name) {
  if (name == "settings") return gear();
  if (name == "audio")
    return fillEO("M3.5 9.2h3.3L11 5.3c.6-.5 1.5-.1 1.5.7v12c0 .8-.9 1.2-1.5.7l-4.2-3.9H3.5c-.6 0-1-.4-1-1v-3.6c0-.6.4-1 "
                  "1-1z") +
           stroke2("M15.6 9.2a4 4 0 0 1 0 5.6") + stroke2("M18.5 6.4a8 8 0 0 1 0 11.2");
  if (name == "mirror")
    return fillEO("M9.6 6.4v12.1H3.9c-.8 0-1.3-.9-.9-1.6l5-10.4c.4-.8 1.6-.6 1.6.1z") +
           stroke2("M14.4 7.1v11.4h5.4c.8 0 1.3-.9.9-1.6z") +
           "<path d=\"M12 3v1.6M12 7.2v1.6M12 11.4v1.6M12 15.6v1.6M12 19.8v1.2\" fill=\"none\" stroke=\"#fff\" "
           "stroke-width=\"1.5\" stroke-linecap=\"round\"/>";
  if (name == "sparkles")
    return fillEO("M10 3.2c.5 3.9 2.4 5.9 6.4 6.4-4 .5-5.9 2.5-6.4 6.4-.5-3.9-2.4-5.9-6.4-6.4 4-.5 5.9-2.5 6.4-6.4z") +
           fillEO("M18.2 13.4c.2 1.7 1.1 2.6 2.8 2.8-1.7.2-2.6 1.1-2.8 2.8-.2-1.7-1.1-2.6-2.8-2.8 1.7-.2 2.6-1.1 2.8-2.8z") +
           fillEO("M18.6 2.6c.1 1 .6 1.5 1.6 1.6-1 .1-1.5.6-1.6 1.6-.1-1-.6-1.5-1.6-1.6 1-.1 1.5-.6 1.6-1.6z");
  if (name == "tv")
    return fillEO(rrect(2.2f, 5.5f, 19.6f, 13.5f, 3.2f) + rrect(4.6f, 7.9f, 14.8f, 8.7f, 1.8f)) + stroke2("M9 21.2h6") +
           stroke2("M8.5 2.3 12 5l3.5-2.7");
  if (name == "depth")
    return fillEO("M12 2.8c.3 0 .5.1.8.2l7.6 4.1c.8.4.8 1.5 0 1.9l-7.6 4.1c-.5.3-1.1.3-1.6 0L3.6 9c-.8-.4-.8-1.5 0-1.9L11.2 "
                  "3c.3-.1.5-.2.8-.2z") +
           stroke2("M3.6 12.6 12 17.1l8.4-4.5") + stroke2("M3.6 16.6 12 21.1l8.4-4.5");
  if (name == "snapshot")
    return fillEO("M8.6 5.6 9.7 3.9c.3-.5.8-.8 1.4-.8h1.8c.6 0 1.1.3 1.4.8l1.1 1.7z" + rrect(2.2f, 5.6f, 19.6f, 14.4f, 3.4f) +
                  circ(12, 12.8f, 4.3f)) +
           fillEO(circ(12, 12.8f, 2.4f)) + fillEO(circ(18.2f, 8.7f, 1.0f));
  if (name == "record") return stroke2(circ(12, 12, 8.8f)) + fillEO(circ(12, 12, 5.4f));
  if (name == "stop") return fillEO(rrect(5.5f, 5.5f, 13, 13, 3.2f));
  if (name == "mic")
    return fillEO(rrect(8.8f, 2.4f, 6.4f, 12.2f, 3.2f)) + stroke2("M18.6 11.2a6.6 6.6 0 0 1-13.2 0") +
           stroke2("M12 17.8v3.2") + stroke2("M9 21h6");
  if (name == "webcam")  // picture-in-picture
    return stroke2(rrect(2.8f, 4.8f, 18.4f, 14.4f, 3.2f)) + fillEO(rrect(11.6f, 11.4f, 7.4f, 5.6f, 1.6f));
  if (name == "fullscreen")
    return stroke2("M3.8 9V6.4a2.6 2.6 0 0 1 2.6-2.6H9") + stroke2("M20.2 9V6.4a2.6 2.6 0 0 0-2.6-2.6H15") +
           stroke2("M3.8 15v2.6a2.6 2.6 0 0 0 2.6 2.6H9") + stroke2("M20.2 15v2.6a2.6 2.6 0 0 1-2.6 2.6H15");
  if (name == "exit-fullscreen")
    return stroke2("M9 3.8v3.4a1.8 1.8 0 0 1-1.8 1.8H3.8") + stroke2("M15 3.8v3.4a1.8 1.8 0 0 0 1.8 1.8h3.4") +
           stroke2("M20.2 15h-3.4a1.8 1.8 0 0 0-1.8 1.8v3.4") + stroke2("M3.8 15h3.4A1.8 1.8 0 0 1 9 16.8v3.4");
  if (name == "vr")
    return fillEO("M3 9.6C3 7.6 4.6 6 6.6 6h10.8c2 0 3.6 1.6 3.6 3.6v3.8c0 2-1.6 3.6-3.6 3.6h-2.5c-.9 0-1.6-.4-2.1-1.2l-.4-.7"
                  "c-.2-.4-.6-.6-1-.6h-.8c-.4 0-.8.2-1 .6l-.4.7c-.5.8-1.2 1.2-2.1 1.2H6.6C4.6 17 3 15.4 3 13.4z" +
                  circ(8, 11.4f, 2.1f) + circ(16, 11.4f, 2.1f));
  if (name == "play")
    return fillEO("M7.5 5.3v13.4c0 .9 1 1.4 1.7.9l9.8-6.7c.6-.4.6-1.4 0-1.8L9.2 4.4c-.7-.5-1.7 0-1.7.9z");
  return "";
}

// ---- line set ----
// UVC Studio's own line icons, drawn on a 24×24 grid: 1.75 px strokes with round
// caps and joins, live area roughly 2.75–21.25 so every glyph has the same
// optical size. A few glyphs mix in solid parts (dots, rounded fills) where that
// reads better at 12–16 px.
const char* SL = "fill=\"none\" stroke=\"#fff\" stroke-width=\"1.75\" stroke-linecap=\"round\" stroke-linejoin=\"round\"";

std::string ln(const std::string& d) { return "<path d=\"" + d + "\" " + SL + "/>"; }
std::string lnDash(const std::string& d, const char* dash) {
  return "<path d=\"" + d + "\" " + SL + " stroke-dasharray=\"" + dash + "\"/>";
}
std::string ring(float cx, float cy, float r) { return ln(circ(cx, cy, r)); }
std::string dot(float cx, float cy, float r) { return "<path d=\"" + circ(cx, cy, r) + "\" fill=\"#fff\"/>"; }
std::string box(float x, float y, float w, float h, float r) { return ln(rrect(x, y, w, h, r)); }
std::string solid(const std::string& d) { return "<path d=\"" + d + "\" fill=\"#fff\"/>"; }
// Filled shape whose corners are softened by a matching round-joined stroke.
std::string soft(const std::string& d) {
  return "<path d=\"" + d + "\" fill=\"#fff\" stroke=\"#fff\" stroke-width=\"1.75\" stroke-linejoin=\"round\"/>";
}

// Cog drawn as one smooth outline: six soft, flat-topped teeth (a squashed
// cosine), so the silhouette stays friendly instead of spiky.
std::string cog() {
  std::string d;
  const int n = 144;
  const float k = 2.4f, norm = std::tanh(k);
  for (int i = 0; i < n; ++i) {
    float a = (float)i / n * 6.2831853f;
    float t = std::tanh(k * std::cos(6 * a)) / norm;
    float r = 7.75f + 1.35f * t;
    d += (i == 0 ? "M" : "L") + num(12 + r * std::cos(a)) + " " + num(12 + r * std::sin(a));
  }
  return ln(d + "z") + ring(12, 12, 2.75f);
}

std::string lineBody(const std::string& name) {
  if (name == "settings") return cog();
  if (name == "audio")  // speaker with two sound waves
    return ln("M4 9.5h2.4l4.3-3.7c.5-.4 1.3-.1 1.3.6v11.2c0 .7-.8 1-1.3.6L6.4 14.5H4c-.6 0-1-.4-1-1v-3c0-.6.4-1 1-1z") +
           ln("M15.25 9.5c1.25 1.4 1.25 3.6 0 5") + ln("M18.25 6.75c2.75 3 2.75 7.5 0 10.5");
  if (name == "mirror")  // dashed axis, outlined shape and its solid reflection
    return lnDash("M12 2.75v18.5", "1.75 2.6") + ln("M9.25 5.75V18.5H3.25z") + soft("M14.75 5.75V18.5h6z");
  if (name == "sparkles")  // one large four-point glint, a small cross and a dot
    return ln("M10.5 5.5Q11 12.5 17.5 13 11 13.5 10.5 20.5 10 13.5 3.5 13 10 12.5 10.5 5.5z") +
           ln("M18.25 3.25v5M15.75 5.75h5") + dot(19.25f, 18.5f, 1.15f);
  if (name == "tv")  // CRT set: barrel-shaped screen, knobs, one antenna, feet
    return box(2.75f, 6.25f, 18.5f, 13.25f, 3) +
           ln("M6.5 9h7.25c.9 0 1.5.6 1.6 1.4.2 1.4.2 3.8 0 5.2-.1.8-.7 1.4-1.6 1.4H6.5c-.9 0-1.5-.6-1.6-1.4-.2-1.4-.2-3.8 "
              "0-5.2.1-.8.7-1.4 1.6-1.4z") +
           dot(18.25f, 10.5f, 1.1f) + ln("M17.6 14.25h1.3") + ln("M11.5 6.25 14.75 3") + dot(15.25f, 2.6f, 1.2f) +
           ln("M6.25 19.5v1.5M17.75 19.5v1.5");
  if (name == "depth")  // three cards receding diagonally
    return box(3, 9, 12, 12, 2.5f) + ln("M6 9v-.5A2.5 2.5 0 0 1 8.5 6h7A2.5 2.5 0 0 1 18 8.5v7a2.5 2.5 0 0 1-2.5 2.5H15") +
           ln("M9 6v-.5A2.5 2.5 0 0 1 11.5 3h7A2.5 2.5 0 0 1 21 5.5v7a2.5 2.5 0 0 1-2.5 2.5H18");
  if (name == "snapshot")  // camera with an offset viewfinder and flash dot
    return box(2.75f, 7, 18.5f, 12.75f, 3.25f) + ln("M6 7V5.75c0-.55.45-1 1-1h3c.55 0 1 .45 1 1V7") +
           ring(12.5f, 13.4f, 3.5f) + dot(17.9f, 10.25f, 1.05f);
  if (name == "record")  // solid core inside an open ring
    return ln("M19.93 8.3A8.75 8.75 0 1 1 14.26 3.55") + dot(12, 12, 4.75f);
  if (name == "stop") return solid(rrect(6, 6, 12, 12, 3));
  if (name == "play") return soft("M8.25 5.75v12.5L18.5 12z");
  if (name == "mic")  // capsule with grille marks, cradle, stand
    return box(8.75f, 2.75f, 6.5f, 11.25f, 3.25f) + ln("M8.75 6.75h2.5M8.75 9.75h2.5") +
           ln("M5.5 11.25a6.5 6.5 0 0 0 13 0") + ln("M12 17.75V21M8.75 21.25h6.5");
  if (name == "webcam")  // overlay: small window inside the frame
    return box(2.75f, 4.75f, 18.5f, 14.5f, 3) + box(11.75f, 11, 6.75f, 5.5f, 1.5f);
  if (name == "fullscreen")
    return ln("M3.75 8.5V6.25a2.5 2.5 0 0 1 2.5-2.5H8.5") + ln("M15.5 3.75h2.25a2.5 2.5 0 0 1 2.5 2.5V8.5") +
           ln("M20.25 15.5v2.25a2.5 2.5 0 0 1-2.5 2.5H15.5") + ln("M8.5 20.25H6.25a2.5 2.5 0 0 1-2.5-2.5V15.5");
  if (name == "exit-fullscreen")
    return ln("M9 3.75V7.25A1.75 1.75 0 0 1 7.25 9H3.75") + ln("M15 3.75v3.5A1.75 1.75 0 0 0 16.75 9h3.5") +
           ln("M20.25 15h-3.5A1.75 1.75 0 0 0 15 16.75v3.5") + ln("M3.75 15h3.5A1.75 1.75 0 0 1 9 16.75v3.5");
  if (name == "vr")  // headset with top strap and solid lenses
    return "<g transform=\"translate(0 .75)\">" +
           ln("M6 8h12a3.25 3.25 0 0 1 3.25 3.25v3.5A3.25 3.25 0 0 1 18 18h-3.1a1.5 1.5 0 0 1-1.3-.75l-.5-.85a1.25 1.25 0 "
              "0 0-2.2 0l-.5.85A1.5 1.5 0 0 1 9.1 18H6a3.25 3.25 0 0 1-3.25-3.25v-3.5A3.25 3.25 0 0 1 6 8z") +
           ln("M7.5 8V6.5a1.75 1.75 0 0 1 1.75-1.75h5.5A1.75 1.75 0 0 1 16.5 6.5V8") +
           solid(rrect(5.75f, 10.75f, 4.25f, 3.25f, 1.4f)) + solid(rrect(14, 10.75f, 4.25f, 3.25f, 1.4f)) + "</g>";
  if (name == "check") return ln("M4.75 12.75 9.5 17.25 19.25 6.75");
  if (name == "close") return ln("M6.5 6.5l11 11M17.5 6.5l-11 11");
  if (name == "chevron") return ln("M6.5 9.75 12 15.25l5.5-5.5");
  if (name == "arrow") return ln("M4.5 12h14.25M13.5 6.5 19 12l-5.5 5.5");
  if (name == "plug")  // connect: tapered plug seated in its socket
    return ln("M2.25 12H5.5") + ln("M5.5 9.5 9.25 7.25h.5c.55 0 1 .45 1 1v7.5c0 .55-.45 1-1 1h-.5L5.5 14.5z") +
           ln("M10.75 10.25H13M10.75 13.75H13") + box(13, 6, 5, 12, 2.5f) + ln("M18 12h3.75");
  if (name == "unplug")  // disconnect: plug pulled out, spark marks in the gap
    return ln("M2.25 12H4") + ln("M4 9.5 7.75 7.25h.5c.55 0 1 .45 1 1v7.5c0 .55-.45 1-1 1h-.5L4 14.5z") +
           ln("M9.25 10.25h2M9.25 13.75h2") + box(15, 6, 4.75f, 12, 2.5f) + ln("M19.75 12h2") +
           ln("M13.1 3.75v1.75M13.1 18.5v1.75");
  if (name == "shield")  // shield with a keyhole
    return ln("M12 3.25c2.4 1.5 4.9 2.25 7.5 2.25v5.75c0 4.9-3.1 8.4-7.5 9.75-4.4-1.35-7.5-4.85-7.5-9.75V5.5c2.6 0 5.1-.75 "
              "7.5-2.25z") +
           dot(12, 10.5f, 1.75f) + ln("M12 11.5v3.75");
  if (name == "swap")  // two opposed vertical arrows
    return ln("M8 19.25V4.75M4.5 8.25 8 4.75l3.5 3.5") + ln("M16 4.75v14.5m-3.5-3.5 3.5 3.5 3.5-3.5");
  if (name == "keyboard") {  // key dots and a space bar
    std::string s = box(2.5f, 5.75f, 19, 12.5f, 2.75f);
    for (int r = 0; r < 2; ++r)
      for (int c = 0; c < 5; ++c) s += dot(6.f + 3 * c, 9.5f + 2.85f * r, 0.95f);
    return s + ln("M8.25 15h7.5");
  }
  if (name == "folder")  // open folder: back panel and a tilted front flap
    return ln("M3.5 17V6.5a2 2 0 0 1 2-2h3.1c.6 0 1.1.3 1.4.7l.9 1.3h7.6a2 2 0 0 1 2 2V9.5") +
           ln("M3.6 18.1 5.1 11c.2-.9 1-1.5 1.9-1.5h12.6c1.2 0 2.1 1.1 1.9 2.3l-1.1 6.2c-.2.9-1 1.5-1.9 1.5H5.5c-1.2 "
              "0-2.1-1.1-1.9-2.3z");
  if (name == "corner")  // overlay corner picker: chosen corner solid, others dotted
    return box(3, 4.5f, 18, 15, 3) + solid(rrect(13.25f, 12, 4.75f, 4.5f, 1.25f)) + dot(7, 8.5f, 1) + dot(17, 8.5f, 1) +
           dot(7, 15.5f, 1);
  if (name == "size")  // small box growing into a dashed larger one
    return box(3.5f, 13, 7.5f, 7.5f, 1.75f) + lnDash(rrect(3.5f, 3.5f, 17, 17, 3), "2.2 2.6") +
           ln("M12.75 11.25 18 6M13.75 6H18v4.25");
  if (name == "globe")  // meridian ellipse and two latitude lines
    return ring(12, 12, 8.75f) +
           ln("M12 3.25c2.3 2.3 3.5 5.2 3.5 8.75s-1.2 6.45-3.5 8.75c-2.3-2.3-3.5-5.2-3.5-8.75s1.2-6.45 3.5-8.75z") +
           ln("M4.3 9h15.4M4.3 15h15.4");
  if (name == "lock")  // padlock: closed shackle, body with a keyhole
    return box(4.75f, 10.5f, 14.5f, 10.5f, 2.75f) + ln("M8 10.5V7.25a4 4 0 0 1 8 0v3.25") + dot(12, 15, 1.35f) +
           ln("M12 15.5v2.25");
  if (name == "unlock")  // padlock: shackle swung open on the right
    return box(4.75f, 10.5f, 14.5f, 10.5f, 2.75f) + ln("M8 10.5V7.25a4 4 0 0 1 7.8-1.25") + dot(12, 15, 1.35f) +
           ln("M12 15.5v2.25");
  if (name == "droplet")  // drop with an inner highlight
    return ln("M12 3.25c3.6 4 6.25 7.6 6.25 10.75a6.25 6.25 0 0 1-12.5 0C5.75 10.85 8.4 7.25 12 3.25z") +
           ln("M9.25 14.25a2.75 2.75 0 0 0 2.5 2.75");
  return "";
}

// Alternative names accepted by icons::draw()/get().
std::string canonical(const std::string& n) {
  static const std::map<std::string, std::string> alias = {
      {"listen", "audio"},        {"speaker", "audio"},       {"upscale", "sparkles"},   {"sparkle", "sparkles"},
      {"crt", "tv"},              {"layers", "depth"},        {"camera", "snapshot"},    {"overlay", "webcam"},
      {"pip", "webcam"},          {"immersive", "vr"},        {"headset", "vr"},         {"connect", "plug"},
      {"disconnect", "unplug"},   {"shortcuts", "keyboard"},  {"shortcut", "keyboard"},  {"language", "globe"},
      {"exitFullscreen", "exit-fullscreen"},                  {"exit_fullscreen", "exit-fullscreen"},
      {"color", "droplet"},       {"locked", "lock"},         {"unlocked", "unlock"},
  };
  auto it = alias.find(n);
  return it == alias.end() ? n : it->second;
}

// "line/<name>" or "glass/<name>" picks a set explicitly; a bare name uses the
// glass glyph when one exists (dock / toolbar) and the line glyph otherwise.
std::string body(const std::string& raw) {
  if (raw.rfind("line/", 0) == 0) return lineBody(canonical(raw.substr(5)));
  if (raw.rfind("glass/", 0) == 0) return glassBody(canonical(raw.substr(6)));
  std::string name = canonical(raw);
  std::string g = glassBody(name);
  return g.empty() ? lineBody(name) : g;
}

struct Key {
  std::string name;
  int px;
  bool operator<(const Key& o) const { return name < o.name || (name == o.name && px < o.px); }
};
std::map<Key, unsigned> gCache;

}  // namespace

namespace icons {

ImTextureID get(const char* name, int px) {
  if (px < 4) px = 4;
  Key k{name, px};
  auto it = gCache.find(k);
  if (it != gCache.end()) return (ImTextureID)(intptr_t)it->second;
  std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"24\" height=\"24\">" +
                    body(name) + "</svg>";
  std::vector<char> buf(svg.begin(), svg.end());
  buf.push_back(0);
  NSVGimage* img = nsvgParse(buf.data(), "px", 96.f);
  std::vector<unsigned char> rgba((size_t)px * px * 4, 0);
  if (img) {
    NSVGrasterizer* r = nsvgCreateRasterizer();
    nsvgRasterize(r, img, 0, 0, px / 24.f, rgba.data(), px, px, px * 4);
    nsvgDeleteRasterizer(r);
    nsvgDelete(img);
  }
  GLuint tex = 0;
  glGenTextures(1, &tex);
  GLint prev;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, px, px, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  glBindTexture(GL_TEXTURE_2D, prev);
  gCache[k] = tex;
  return (ImTextureID)(intptr_t)tex;
}

void draw(ImDrawList* dl, const char* name, ImVec2 center, float size, ImU32 col, float fbScale) {
  int px = (int)(size * fbScale + 0.5f);
  ImTextureID t = get(name, px);
  ImVec2 a(center.x - size * 0.5f, center.y - size * 0.5f);
  // Snap to the pixel grid for crispness.
  a.x = (float)(int)(a.x * fbScale + 0.5f) / fbScale;
  a.y = (float)(int)(a.y * fbScale + 0.5f) / fbScale;
  dl->AddImage(ImTextureRef(t), a, ImVec2(a.x + size, a.y + size), ImVec2(0, 0), ImVec2(1, 1), col);
}

void clear() {
  for (auto& [k, t] : gCache) {
    GLuint id = t;
    glDeleteTextures(1, &id);
  }
  gCache.clear();
}

}  // namespace icons
