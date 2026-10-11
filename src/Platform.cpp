#include "Platform.h"

#include "version.h"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <shellapi.h>
#include <direct.h>
#else
#include <dlfcn.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace platform {

std::string joinPath(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
#ifdef _WIN32
  char sep = '\\';
#else
  char sep = '/';
#endif
  if (a.back() == '/' || a.back() == '\\') return a + b;
  return a + sep + b;
}

static void mkdirs(const std::string& p) {
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::u8path(p), ec);
}

#ifdef _WIN32
static std::string narrow(const wchar_t* w) {
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
  std::string s(n > 0 ? n - 1 : 0, '\0');
  if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
  return s;
}
static std::string knownFolder(REFKNOWNFOLDERID id) {
  PWSTR p = nullptr;
  std::string out;
  if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) out = narrow(p);
  if (p) CoTaskMemFree(p);
  return out;
}
#else
static std::string home() {
  const char* h = getenv("HOME");
  if (h && *h) return h;
  if (passwd* pw = getpwuid(getuid())) return pw->pw_dir;
  return "/tmp";
}
#endif

std::string configDir() {
  std::string d;
#if defined(_WIN32)
  d = joinPath(knownFolder(FOLDERID_RoamingAppData), "UVC Studio");
#else
  const char* x = getenv("XDG_CONFIG_HOME");
  d = joinPath(x && *x ? std::string(x) : joinPath(home(), ".config"), "uvc-studio");
#endif
  mkdirs(d);
  return d;
}

#if !defined(_WIN32)
// XDG user directory (`XDG_PICTURES_DIR`, `XDG_VIDEOS_DIR`): environment
// first, then ~/.config/user-dirs.dirs, then ~/<fallback>.
static std::string xdgUserDir(const char* var, const char* fallback) {
  const char* env = getenv(var);
  if (env && *env) return env;
  const char* cfg = getenv("XDG_CONFIG_HOME");
  std::string file = joinPath(cfg && *cfg ? std::string(cfg) : joinPath(home(), ".config"), "user-dirs.dirs");
  std::ifstream in(file);
  std::string line, prefix = std::string(var) + "=";
  while (std::getline(in, line)) {
    if (line.rfind(prefix, 0) != 0) continue;
    std::string v = line.substr(prefix.size());
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"') v = v.substr(1, v.size() - 2);
    if (v.rfind("$HOME", 0) == 0) v = home() + v.substr(5);
    // "$HOME/" alone means the directory is disabled.
    if (!v.empty() && v != home() && v != home() + "/") return v;
    break;
  }
  return joinPath(home(), fallback);
}
#endif

// <Pictures or Videos>/UVC Studio, created on demand. Quest: the app's own
// external folder; publishOutput() then copies the file into the gallery.
static std::string outputDir(bool video) {
  std::string d;
#if defined(_WIN32)
  std::string base = knownFolder(video ? FOLDERID_Videos : FOLDERID_Pictures);
  if (base.empty()) base = joinPath(getenv("USERPROFILE") ? getenv("USERPROFILE") : "C:\\", video ? "Videos" : "Pictures");
  d = joinPath(base, "UVC Studio");
#else
  d = joinPath(video ? xdgUserDir("XDG_VIDEOS_DIR", "Videos") : xdgUserDir("XDG_PICTURES_DIR", "Pictures"),
               "UVC Studio");
#endif
  mkdirs(d);
  return d;
}

std::string picturesDir() { return outputDir(false); }
std::string videosDir() { return outputDir(true); }

std::string newOutputPath(const std::string& dir, const std::string& ext) {
  std::time_t now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  char stamp[32];
  std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H.%M.%S", &local);
  std::string base = std::string("UVC Studio ") + stamp;
  std::string path = joinPath(dir, base + ext);
  std::error_code ec;
  for (int n = 2; std::filesystem::exists(std::filesystem::u8path(path), ec); ++n)
    path = joinPath(dir, base + " (" + std::to_string(n) + ")" + ext);
  return path;
}

std::string assetsDir() {
  static std::string cached;
  if (!cached.empty()) return cached;
  char* b = SDL_GetBasePath();
  std::string base = b ? b : "./";
  SDL_free(b);
  const char* candidates[] = {"assets", "../share/uvc-studio", "../assets", "../../assets"};
  for (const char* c : candidates) {
    std::string p = joinPath(base, c);
    std::string probe = joinPath(joinPath(p, "fonts"), "Manrope-400.ttf");
    std::error_code ec;
    if (std::filesystem::exists(std::filesystem::u8path(probe), ec)) {
      cached = p;
      return cached;
    }
  }
  cached = joinPath(base, "assets");
  return cached;
}

std::string versionLabel() { return std::string("v") + UVC_VERSION_NAME; }

// Desktop DPI scale as configured by the user (1.0 = 96 dpi).
static float desktopDpiScale(int displayIndex) {
#ifdef _WIN32
  float ddpi, hdpi, vdpi;
  if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0 && hdpi > 0) return hdpi / 96.f;
  return 1.f;
#else
  (void)displayIndex;
  static float cached = [] {
    float s = 1.f;
    for (const char* v : {"GDK_SCALE", "QT_SCALE_FACTOR"}) {
      const char* e = getenv(v);
      if (e && *e) s = std::max(s, (float)atof(e));
    }
    // Xft.dpi from the X resource database (set by GNOME/KDE/Xfce scaling).
    if (void* x11 = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL)) {
      using OpenFn = void* (*)(const char*);
      using ResFn = char* (*)(void*);
      using CloseFn = int (*)(void*);
      auto xopen = (OpenFn)dlsym(x11, "XOpenDisplay");
      auto xres = (ResFn)dlsym(x11, "XResourceManagerString");
      auto xclose = (CloseFn)dlsym(x11, "XCloseDisplay");
      if (xopen && xres && xclose) {
        if (void* dpy = xopen(nullptr)) {
          if (const char* r = xres(dpy)) {
            const char* p = strstr(r, "Xft.dpi:");
            if (p) {
              float dpi = (float)atof(p + 8);
              if (dpi > 0) s = std::max(s, dpi / 96.f);
            }
          }
          xclose(dpy);
        }
      }
      dlclose(x11);
    }
    return s;
  }();
  return cached;
#endif
}

float resolutionScale(int displayIndex) {
  SDL_Rect b;
  if (displayIndex < 0 || SDL_GetDisplayBounds(displayIndex, &b) != 0 || b.w <= 0 || b.h <= 0) return 1.f;
  // Treat portrait screens like their landscape equivalent.
  int w = std::max(b.w, b.h), h = std::min(b.w, b.h);
  float s = std::min(w / 1920.f, h / 1080.f);
  s = std::round(s * 20.f) / 20.f;  // 5% steps
  return std::clamp(s, 1.f, 4.f);
}

float displayScale(int displayIndex) {
  if (const char* e = getenv("UVC_STUDIO_SCALE")) {
    float v = (float)atof(e);
    if (v > 0.3f && v < 6.f) return v;
  }
  return std::max(resolutionScale(displayIndex), desktopDpiScale(displayIndex));
}

void openUrl(const std::string& url) { SDL_OpenURL(url.c_str()); }

void openFolder(const std::string& dir) {
  std::string url = "file://";
#ifdef _WIN32
  url += "/";
#endif
  for (char ch : dir) {
    unsigned char c = (unsigned char)ch;
    if (c == '\\') {
      url += '/';
    } else if (c == ' ' || c == '%' || c == '#' || c == '?') {
      char hex[4];
      std::snprintf(hex, sizeof hex, "%%%02X", c);
      url += hex;
    } else {
      url += ch;
    }
  }
  SDL_OpenURL(url.c_str());
}

void requestAudioPermission() {
}

std::string publishOutput(const std::string& path, const std::string& mime) {
  (void)mime;
  return path;
}

void revealInFolder(const std::string& path) {
#ifdef _WIN32
  std::string cmd = "/select,\"" + path + "\"";
  int n = MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, nullptr, 0);
  std::wstring w(n, 0);
  MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, w.data(), n);
  ShellExecuteW(nullptr, L"open", L"explorer.exe", w.c_str(), nullptr, SW_SHOWNORMAL);
#else
  std::string dir = path;
  auto p = dir.find_last_of('/');
  if (p != std::string::npos) dir = dir.substr(0, p);
  std::string url = "file://";
  for (unsigned char c : dir) {
    if (isalnum(c) || strchr("/-_.~", c) || c >= 0x80)
      url += (char)c;
    else {
      char b[4];
      std::snprintf(b, sizeof b, "%%%02X", c);
      url += b;
    }
  }
  SDL_OpenURL(url.c_str());
#endif
}

std::string preferredLanguage() {
#if defined(_WIN32)
  wchar_t name[LOCALE_NAME_MAX_LENGTH] = {0};
  if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) > 0) return narrow(name);
  return "";
#else
  // POSIX precedence: LC_ALL, then LC_MESSAGES, then LANG. LANGUAGE may hold
  // a priority list ("zh_TW:en"); its first entry wins when set.
  for (const char* var : {"LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"}) {
    const char* v = getenv(var);
    if (!v || !*v) continue;
    std::string s = v;
    s = s.substr(0, s.find(':'));
    if (s.empty() || s == "C" || s == "POSIX" || s.rfind("C.", 0) == 0) continue;
    return s;
  }
  return "";
#endif
}

}  // namespace platform

void Settings::load() {
  path_ = platform::joinPath(platform::configDir(), "settings.ini");
  std::ifstream f(std::filesystem::u8path(path_));
  std::string line;
  while (std::getline(f, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    kv_[line.substr(0, eq)] = line.substr(eq + 1);
  }
}

void Settings::save() const {
  if (path_.empty()) return;
  auto tmp = std::filesystem::u8path(path_ + ".tmp");
  {
    std::ofstream f(tmp, std::ios::trunc);
    for (auto& [k, v] : kv_) f << k << '=' << v << '\n';
  }
  std::error_code ec;
  std::filesystem::rename(tmp, std::filesystem::u8path(path_), ec);
}

std::string Settings::get(const std::string& k, const std::string& def) const {
  auto it = kv_.find(k);
  return it == kv_.end() ? def : it->second;
}

double Settings::getNum(const std::string& k, double def) const {
  auto it = kv_.find(k);
  if (it == kv_.end()) return def;
  char* end = nullptr;
  double v = strtod(it->second.c_str(), &end);
  return (end && end != it->second.c_str()) ? v : def;
}

void Settings::set(const std::string& k, const std::string& v) {
  kv_[k] = v;
  save();
}

void Settings::setNum(const std::string& k, double v) {
  char b[64];
  std::snprintf(b, sizeof b, "%.4f", v);
  set(k, b);
}
