/**
 * @file CameraManager.cpp
 * @brief Implementation of the camera manager with a dual backend.
 */

#include "camera/CameraManager.hpp"

#include <opencv2/videoio.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>

#include "utils/Logger.hpp"

namespace fishcam {

namespace {

/**
 * @brief Runs a shell command and returns its standard output.
 */
std::string RunCommand(const std::string& command) {
  std::string output;
  FILE* pipe = ::popen(command.c_str(), "r");
  if (pipe == nullptr) {
    return output;
  }
  char buffer[256];
  while (::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  ::pclose(pipe);
  return output;
}

}  // namespace

CameraManager::CameraManager(const Config& config) : config_(config) {}

CameraManager::~CameraManager() {
  Shutdown();
}

void CameraManager::SetLastError(const std::string& message) {
  last_error_ = message;
}

std::string CameraManager::GetLastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_error_;
}

void CameraManager::ApplyPropertiesLocked() {
#ifdef FISH_CAM_USE_OPENCV_BACKEND
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
  capture_.set(cv::CAP_PROP_FPS, config_.frame_rate);
  // Do NOT force MJPG: the libcamera V4L2 compatibility layer on Bookworm+
  // only exposes NV21/YUYV; requesting MJPEG makes the stream never start and
  // VideoCapture::read() blocks forever.
  if (!capture_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('N', 'V', '2', '1'))) {
    capture_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
  }
#if defined(CV_VERSION_MAJOR) && (CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 6))
  // NOTE: only honored by the FFmpeg backend; V4L2 read() is covered by the
  // timeout in Capture().
  capture_.set(cv::CAP_PROP_READ_TIMEOUT_MSEC, config_.capture_timeout_ms);
#endif
#else
  camera_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
  camera_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
  camera_.set(cv::CAP_PROP_FPS, config_.frame_rate);
  camera_.set(cv::CAP_PROP_BRIGHTNESS, config_.brightness);
  camera_.set(cv::CAP_PROP_CONTRAST, config_.contrast);
  camera_.set(cv::CAP_PROP_SATURATION, config_.saturation);
  if (config_.iso > 0) {
    // RaspiCam_Cv maps CAP_PROP_GAIN [0,100] to ISO [0,800].
    camera_.set(cv::CAP_PROP_GAIN, config_.iso * 100.0 / 800.0);
  }
  // RaspiCam_Cv only supports auto exposure or a fixed shutter speed; any
  // non-auto mode (night, sports, ...) falls back to automatic exposure.
  camera_.set(cv::CAP_PROP_EXPOSURE, 0);
  if (config_.exposure != "auto") {
    Logger::Warning("raspicam backend only supports auto exposure; ignoring "
                    "exposure mode \"" +
                    config_.exposure + "\"");
  }
  // sharpness, quality, encoding and the preview window are not exposed by
  // RaspiCam_Cv; sharpness stays at its default and the rest are handled
  // downstream by the image processor.
  camera_.setRotation(config_.rotation);
  camera_.setHorizontalFlip(config_.horizontal_flip);
  camera_.setVerticalFlip(config_.vertical_flip);
#endif
}

bool CameraManager::Initialize() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialized_) {
    return ready_;
  }
  initialized_ = true;

  Logger::Info("Initializing camera " + std::to_string(config_.width) + "x" +
               std::to_string(config_.height) + " @" +
               std::to_string(config_.frame_rate) + " fps, quality " +
               std::to_string(config_.quality));

  ApplyPropertiesLocked();

  const int attempts = std::max(1, config_.retry_attempts);
  for (int attempt = 1; attempt <= attempts; ++attempt) {
#ifdef FISH_CAM_USE_OPENCV_BACKEND
    ready_ = capture_.open(0, cv::CAP_V4L2);
#else
    ready_ = camera_.open();
#endif
    if (ready_) {
      break;
    }
    Logger::Warning("Camera open attempt " + std::to_string(attempt) + "/" +
                    std::to_string(attempts) + " failed");
    if (attempt < attempts) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500 * attempt));
    }
  }

  if (ready_) {
    ApplyPropertiesLocked();
    Logger::Info("Camera opened successfully (OV5647, 130-degree lens)");
    SetLastError("no error");
  } else {
#ifdef FISH_CAM_USE_OPENCV_BACKEND
    SetLastError("camera_open_failed: no V4L2 camera found. On Bookworm use "
                 "the libcamera v4l2 compat layer (docs/INSTALL.md).");
#else
    SetLastError("camera_open_failed: OV5647 not reachable. Check the "
                 "connection and the legacy camera stack (docs/INSTALL.md).");
#endif
    Logger::Error(GetLastError());
  }
  return ready_;
}

bool CameraManager::IsReady() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return ready_;
}

bool CameraManager::Capture(cv::Mat& frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ready_) {
    SetLastError("capture_failed: camera is not ready");
    Logger::Warning(GetLastError());
    return false;
  }
#ifdef FISH_CAM_USE_OPENCV_BACKEND
  // VideoCapture::read() on the V4L2 backend blocks indefinitely when the
  // stream stalls (CAP_PROP_READ_TIMEOUT_MSEC only affects FFmpeg). Run it on
  // a detached worker so we can enforce capture_timeout_ms ourselves.
  cv::Mat read_frame;
  std::atomic<bool> read_done{false};
  std::thread reader([this, &read_frame, &read_done]() {
    if (capture_.read(read_frame)) {
      read_done = true;
    }
  });
  reader.detach();
  const auto deadline =
      std::chrono::steady_clock::now() +
      std::chrono::milliseconds(config_.capture_timeout_ms);
  while (!read_done && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (!read_done) {
    SetLastError("capture_timeout: no frame within " +
                 std::to_string(config_.capture_timeout_ms) + " ms");
    Logger::Error(GetLastError());
    return false;
  }
  frame = read_frame;
#else
  if (!camera_.grab()) {
    SetLastError("capture_timeout: no frame within " +
                 std::to_string(config_.capture_timeout_ms) + " ms");
    Logger::Error(GetLastError());
    return false;
  }
  camera_.retrieve(frame);
#endif
  if (frame.empty()) {
    SetLastError("capture_failed: retrieve() returned an empty frame");
    Logger::Error(GetLastError());
    return false;
  }
  SetLastError("no error");
  return true;
}

bool CameraManager::CaptureAsync(std::function<void(bool, cv::Mat)> callback) {
  if (!IsReady()) {
    SetLastError("async_capture_failed: camera is not ready");
    Logger::Warning(GetLastError());
    return false;
  }
  std::thread worker([this, callback]() {
    cv::Mat frame;
    const bool ok = Capture(frame);
    if (callback) {
      callback(ok, frame);
    }
  });
  worker.detach();
  Logger::Debug("Asynchronous capture started");
  return true;
}

int CameraManager::CaptureBurst(int count, std::vector<cv::Mat>& frames) {
  frames.clear();
  if (count <= 0) {
    Logger::Warning("CaptureBurst called with invalid count " +
                    std::to_string(count));
    return 0;
  }

  frames.reserve(count);
  for (int i = 0; i < count; ++i) {
    cv::Mat frame;
    if (!Capture(frame)) {
      break;
    }
    frames.push_back(frame);
  }
  Logger::Info("Burst finished: " + std::to_string(frames.size()) + "/" +
               std::to_string(count) + " frames captured");
  return static_cast<int>(frames.size());
}

void CameraManager::Shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (ready_ || initialized_) {
#ifdef FISH_CAM_USE_OPENCV_BACKEND
    capture_.release();
#else
    camera_.release();
#endif
    ready_ = false;
    initialized_ = false;
    Logger::Info("Camera released");
  }
}

CameraManager::Config CameraManager::GetConfig() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return config_;
}

std::vector<std::string> CameraManager::DetectCameras() {
  std::vector<std::string> devices;
  for (int index = 0; index < 32; ++index) {
    const std::string path = "/dev/video" + std::to_string(index);
    if (std::filesystem::exists(path)) {
      devices.push_back(path);
    }
  }

  const std::string camera_report =
      RunCommand("vcgencmd get_camera 2>/dev/null");
  if (!camera_report.empty()) {
    if (camera_report.find("supported=1") != std::string::npos) {
      Logger::Info("Legacy camera stack: " + camera_report);
    } else {
      Logger::Warning("Legacy camera stack reports: " + camera_report);
    }
  }
  return devices;
}

}  // namespace fishcam
