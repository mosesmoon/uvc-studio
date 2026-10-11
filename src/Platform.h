#pragma once
#include <map>
#include <string>

namespace platform {
std::string configDir();     // created if missing
// Snapshot / recording folders, created if missing: Linux
// $XDG_PICTURES_DIR/UVC Studio and $XDG_VIDEOS_DIR/UVC Studio (falling back
// to ~/Pictures, ~/Videos), Windows the Pictures / Videos known folders +
// \UVC Studio. Quest: the app's own folder (publishOutput() moves the file
// into the gallery).
std::string picturesDir();
std::string videosDir();
// "<dir>/UVC Studio YYYY-MM-DD HH.MM.SS<ext>" in local time, with " (2)",
// " (3)" … added when that file already exists.
std::string newOutputPath(const std::string& dir, const std::string& ext);
std::string assetsDir();     // bundled fonts / images
std::string versionLabel();  // "v1.1.0" (from ../VERSION)
void openUrl(const std::string& url);
// Makes a finished snapshot/recording visible to the user. Desktop: no-op
// (returns `path`). Quest: copies it into the shared gallery (Movies/Pictures
// "UVC Studio") and returns that location, or "" on failure.
std::string publishOutput(const std::string& path, const std::string& mime);
// Quest: asks for RECORD_AUDIO (USB audio capture). Desktop: no-op.
void requestAudioPermission();
void revealInFolder(const std::string& path);
// Opens a folder in the system file manager.
void openFolder(const std::string& dir);
std::string joinPath(const std::string& a, const std::string& b);
// OS locale name ("zh_TW.UTF-8", "zh-Hant-TW", "ja-JP" …), "" when unknown.
std::string preferredLanguage();
// Automatic UI scale for a display: grows with resolution (1080p → 1.0,
// 1440p → 1.33, 2160p → 2.0) and never goes below the desktop's own DPI
// scaling (Windows display scaling, Xft.dpi / GDK_SCALE on Linux).
float displayScale(int displayIndex);
// Just the resolution-based part, for display in the settings.
float resolutionScale(int displayIndex);
}  // namespace platform

// Small persistent key/value store (settings.ini in configDir()); keys use
// the `section.name` form.
class Settings {
 public:
  void load();
  void save() const;
  std::string get(const std::string& k, const std::string& def = "") const;
  double getNum(const std::string& k, double def) const;
  void set(const std::string& k, const std::string& v);
  void setNum(const std::string& k, double v);

 private:
  std::map<std::string, std::string> kv_;
  std::string path_;
};
