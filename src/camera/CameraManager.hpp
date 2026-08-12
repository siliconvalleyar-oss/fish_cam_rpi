/**
 * @file CameraManager.hpp
 * @brief Wraps the OV5647 camera through a configurable backend.
 *
 * Two backends are supported (selected at build time):
 *  - raspicam (default): classic MMAL-based library. Used when the legacy
 *    camera stack is available (Raspberry Pi OS Bullseye or older, /opt/vc).
 *  - OpenCV V4L2 (FISH_CAM_USE_OPENCV_BACKEND): used when raspicam is not
 *    present (Raspberry Pi OS Bookworm and later, which only ships libcamera).
 *    Enable the V4L2 compatibility layer at runtime with:
 *    LD_PRELOAD=/usr/libexec/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libcamera/v4l2-compat.so
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#ifndef FISH_CAM_CAMERA_CAMERAMANAGER_HPP_
#define FISH_CAM_CAMERA_CAMERAMANAGER_HPP_

#include <opencv2/core.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#ifndef FISH_CAM_USE_OPENCV_BACKEND
#include <raspicam/raspicam_cv.h>
#else
#include <opencv2/videoio.hpp>
#endif

namespace fishcam {

/**
 * @brief Owns the camera resource (RAII) and exposes a safe capture API.
 *
 * The class follows the RAII pattern: the destructor always releases the
 * camera. Initialization performs up to 3 connection attempts and every
 * capture has a 5 second timeout. All capture methods are thread-safe.
 */
class CameraManager {
 public:
  /** @brief Output encoding of the sensor. */
  enum class Encoding {
    kJpeg = 0,
    kPng = 1,
  };

  /** @brief Camera configuration parameters. */
  struct Config {
    int width = 1920;
    int height = 1080;
    int frame_rate = 30;
    int quality = 100;            ///< Encoder quality (1..100).
    int iso = 0;                  ///< 0 = automatic, otherwise a numeric ISO.
    int brightness = 50;          ///< 0..100
    int contrast = 50;            ///< 0..100
    int saturation = 50;          ///< 0..100
    int sharpness = 50;           ///< -100..100
    std::string exposure = "auto";///< "auto" | "off" | "night" | "backlight"
    int rotation = 0;             ///< Degrees, multiples of 90.
    bool horizontal_flip = false;
    bool vertical_flip = false;
    bool show_preview = false;
    int capture_timeout_ms = 5000;///< Maximum time to wait for a frame.
    int retry_attempts = 3;       ///< Connection attempts on startup.
    Encoding encoding = Encoding::kJpeg;
  };

  /**
   * @brief Constructs the manager with the given configuration.
   * @param config Camera parameters.
   */
  explicit CameraManager(const Config& config);

  /** @brief Releases the camera resource (RAII). */
  ~CameraManager();

  CameraManager(const CameraManager&) = delete;
  CameraManager& operator=(const CameraManager&) = delete;

  /**
   * @brief Opens the camera with retries (default: 3 attempts).
   * @return true if the camera was opened successfully.
   */
  bool Initialize();

  /** @brief Returns whether the camera is ready for captures. */
  bool IsReady() const;

  /**
   * @brief Performs a synchronous (blocking) capture with a timeout.
   * @param frame Output frame in BGR format.
   * @return true on success.
   */
  bool Capture(cv::Mat& frame);

  /**
   * @brief Performs an asynchronous capture on a worker thread.
   * @param callback Invoked on the worker thread with the capture result.
   * @return true if the capture was scheduled.
   */
  bool CaptureAsync(std::function<void(bool, cv::Mat)> callback);

  /**
   * @brief Captures a burst of consecutive images.
   * @param count Number of frames to capture.
   * @param frames Output container (resized to the number captured).
   * @return Number of frames actually captured.
   */
  int CaptureBurst(int count, std::vector<cv::Mat>& frames);

  /**
   * @brief Releases the camera. Idempotent and safe to call multiple times.
   */
  void Shutdown();

  /** @brief Returns a copy of the active configuration. */
  Config GetConfig() const;

  /** @brief Returns the last error message produced by this class. */
  std::string GetLastError() const;

  /**
   * @brief Detects available camera devices.
   * @return A list of detected /dev/video* devices (legacy-stack aware).
   */
  static std::vector<std::string> DetectCameras();

 private:
  void SetLastError(const std::string& message);
  void ApplyPropertiesLocked();

  Config config_;
#ifdef FISH_CAM_USE_OPENCV_BACKEND
  cv::VideoCapture capture_;
#else
  raspicam::RaspiCam_Cv camera_;
#endif
  mutable std::mutex mutex_;
  std::string last_error_;
  bool ready_{false};
  bool initialized_{false};
};

}  // namespace fishcam

#endif  // FISH_CAM_CAMERA_CAMERAMANAGER_HPP_
