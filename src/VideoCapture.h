// Cross-platform video capture device access. Implementations:
//   Linux   — VideoCapture_v4l2.cpp (Video4Linux2, any UVC / V4L2 capture card)
//   Windows — VideoCapture_mf.cpp   (Media Foundation, any UVC / AVStream card)
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Frame.h"

class SampleRing;

// Four-character pixel format codes we understand (V4L2 values; the Windows
// backend maps its subtypes onto the same codes).
constexpr uint32_t fourcc(char a, char b, char c, char d) {
  return (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
}
constexpr uint32_t FCC_YUYV = fourcc('Y', 'U', 'Y', 'V');
constexpr uint32_t FCC_UYVY = fourcc('U', 'Y', 'V', 'Y');
constexpr uint32_t FCC_NV12 = fourcc('N', 'V', '1', '2');
constexpr uint32_t FCC_NV21 = fourcc('N', 'V', '2', '1');
constexpr uint32_t FCC_I420 = fourcc('Y', 'U', '1', '2');
constexpr uint32_t FCC_YV12 = fourcc('Y', 'V', '1', '2');
constexpr uint32_t FCC_MJPG = fourcc('M', 'J', 'P', 'G');
constexpr uint32_t FCC_JPEG = fourcc('J', 'P', 'E', 'G');
constexpr uint32_t FCC_H264 = fourcc('H', '2', '6', '4');
constexpr uint32_t FCC_HEVC = fourcc('H', 'E', 'V', 'C');
constexpr uint32_t FCC_RGB3 = fourcc('R', 'G', 'B', '3');
constexpr uint32_t FCC_BGR3 = fourcc('B', 'G', 'R', '3');
constexpr uint32_t FCC_BGRA = fourcc('A', 'R', '2', '4');  // V4L2_PIX_FMT_ABGR32 (B,G,R,A in memory)
constexpr uint32_t FCC_BGRX = fourcc('X', 'R', '2', '4');  // V4L2_PIX_FMT_XBGR32
constexpr uint32_t FCC_GREY = fourcc('G', 'R', 'E', 'Y');

bool fourccIsCompressed(uint32_t f);
bool fourccSupported(uint32_t f);
std::string fourccName(uint32_t f);
// "Uncompressed" / "MJPEG" / "H.264" ... for the UI.
std::string fourccFriendly(uint32_t f);

struct VideoMode {
  int width = 0;
  int height = 0;
  uint32_t fourcc = 0;
  std::vector<double> fps;  // supported frame rates, descending
};

struct VideoDeviceInfo {
  std::string id;     // stable identifier (bus path / symbolic link)
  std::string name;   // human readable card name (" (2)" appended on duplicates)
  std::string path;   // what the backend opens (/dev/videoN, MF symbolic link)
  std::string driver; // kernel driver name (Linux, e.g. "uvcvideo", "v4l2 loopback"); may be empty
  std::vector<VideoMode> modes;

  // Distinct resolutions (descending by pixel count).
  std::vector<std::pair<int, int>> resolutions() const;
  // Frame rates available at a resolution (any pixel format), descending.
  std::vector<int> frameRates(int w, int h) const;
  int maxWidth() const;
  int maxHeight() const;
  double maxFps() const;
};

std::vector<VideoDeviceInfo> enumerateVideoDevices();
// Android: ask for USB access to every attached UVC device (no-op elsewhere).
void requestVideoDevicePermissions();
// Cheap fingerprint of the set of attached video devices (hotplug detection).
std::string videoDeviceSignature();

// Synthetic device for testing without hardware (UVC_STUDIO_TEST_PATTERN=1).
bool testPatternEnabled();
VideoDeviceInfo testPatternDevice(int index = 0);

// Special `preferFourcc` values for chooseMode().
constexpr uint32_t PREFER_AUTO = 0;
constexpr uint32_t PREFER_UNCOMPRESSED = 1;
constexpr uint32_t PREFER_COMPRESSED = 2;

struct StreamSettings {
  int width = 0;
  int height = 0;
  double fps = 0;
  uint32_t fourcc = 0;
};

// Pick the best concrete mode for a requested resolution / fps. `preferFourcc`
// of 0 means automatic (uncompressed when the device offers it at that
// resolution+rate, otherwise MJPEG, otherwise anything decodable).
StreamSettings chooseMode(const VideoDeviceInfo& dev, int w, int h, double fps, uint32_t preferFourcc);

class VideoCapture {
 public:
  VideoCapture();
  ~VideoCapture();
  VideoCapture(const VideoCapture&) = delete;
  VideoCapture& operator=(const VideoCapture&) = delete;

  bool open(const VideoDeviceInfo& dev, const StreamSettings& want, std::string& err);
  // Quest: also take over and capture the device's own USB audio on open().
  void setDeviceAudio(bool on, int channels = 2) {
    wantDeviceAudio_ = on;
    deviceAudioWantCh_ = channels;
  }
  void close();
  bool isOpen() const { return running_.load(); }

  // Newest frame if its sequence number differs from `lastSeq`.
  FramePtr latest(uint64_t lastSeq);
  StreamSettings actual() const;
  std::string deviceName() const { return name_; }
  // Set when the device vanished or failed mid-stream.
  bool failed() const { return failed_.load(); }
  std::string failure() const;
  // Measured delivery rate.
  double measuredFps() const { return measuredFps_.load(); }
  // Time (nowSeconds) of the newest delivered frame, 0 = none yet.
  double lastFrameTime() const { return lastFrameT_.load(); }
  // Sound captured together with the video (Quest: the capture card's USB
  // audio, 48 kHz interleaved float, deviceAudioChannels() channels: 2, or
  // 6 for 5.1). Null when the backend has none.
  std::shared_ptr<SampleRing> deviceAudio() const;
  int deviceAudioChannels() const;
  std::string deviceAudioInfo() const;

  // ---- backend-internal (called from the capture thread) ----
  struct Impl;
  Impl* implPtr() { return impl_.get(); }
  void publish(FramePtr f);
  FramePtr acquireFrame();
  void reportFailure(const std::string& msg);

 private:
  bool openTestPattern(const StreamSettings& want);
  void closeLocked();  // Quest backend: close() with the USB open lock held
  void closeTestPattern();
  std::thread genThread_;
  std::atomic<bool> genStop_{false};

  std::unique_ptr<Impl> impl_;
  std::atomic<bool> running_{false};
  std::atomic<bool> failed_{false};
  std::atomic<double> measuredFps_{0.0};
  std::atomic<double> lastFrameT_{0.0};
  bool wantDeviceAudio_ = false;
  int deviceAudioWantCh_ = 2;
  mutable std::mutex mu_;
  FramePtr latest_;
  std::vector<FramePtr> pool_;
  uint64_t seq_ = 0;
  double statStart_ = 0;
  int statCount_ = 0;
  StreamSettings actual_;
  std::string name_;
  std::string failure_;
};
