/**
 * @file CameraManager.cpp
 * @brief Implementation of the camera manager using raspicam.
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

raspicam::RASPICAM_EXPOSURE ToRaspiExposure(const std::string& mode) {
  if (mode == "off") return raspicam::RASPICAM_EXPOSURE_OFF;
  if (mode == "night") return raspicam::RASPICAM_EXPOSURE_NIGHT;
  if (mode == "backlight") return raspicam::RASPICAM_EXPOSURE_BACKLIGHT;
  if (mode == "spotlight") return raspicam::RASPICAM_EXPOSURE_SPOTLIGHT;
  if (mode == "sports") return raspicam::RASPICAM_EXPOSURE_SPORTS;
  if (mode == "snow") return raspicam::RASPICAM_EXPOSURE_SNOW;
  if (mode == "beach") return raspicam::RASPICAM_EXPOSURE_BEACH;
  if (mode == "verylong") return raspicam::RASPICAM_EXPOSURE_VERYLONG;
  if (mode == "antishake") return raspicam::RASPICAM_EXPOSURE_ANTISHAKE;
  if (mode == "fireworks") return raspicam::RASPICAM_EXPOSURE_FIREWORKS;
  return raspicam::RASPICAM_EXPOSURE_AUTO;
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
  camera_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
  camera_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
  camera_.setFrameRate(config_.frame_rate);
  if (config_.iso > 0) {
    camera_.setISO(config_.iso);
  }
  camera_.setBrightness(config_.brightness);
  camera_.setContrast(config_.contrast);
  camera_.setSaturation(config_.saturation);
  camera_.setSharpness(config_.sharpness);
  camera_.setRotation(config_.rotation);
  camera_.setHorizontalFlip(config_.horizontal_flip);
  camera_.setVerticalFlip(config_.vertical_flip);
  camera_.setExposure(ToRaspiExposure(config_.exposure));
  camera_.setTimeout(config_.capture_timeout_ms);
  camera_.setEncoding(config_.encoding == Encoding::kPng
                          ? raspicam::RASPICAM_ENCODING_PNG
                          : raspicam::RASPICAM_ENCODING_JPEG);
  camera_.setQuality(config_.quality);
  camera_.setPreview(config_.show_preview);
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
    ready_ = camera_.open(cv::Size(config_.width, config_.height));
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
    Logger::Info("Camera opened successfully (OV5647, 130-degree lens)");
    SetLastError("no error");
  } else {
    SetLastError("camera_open_failed: OV5647 not reachable. Check the "
                 "connection and the legacy camera stack (docs/INSTALL.md).");
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
  if (!camera_.grab()) {
    SetLastError("capture_timeout: no frame within " +
                 std::to_string(config_.capture_timeout_ms) + " ms");
    Logger::Error(GetLastError());
    return false;
  }
  camera_.retrieve(frame);
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
    camera_.release();
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
