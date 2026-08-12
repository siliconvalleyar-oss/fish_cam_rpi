/**
 * @file main.cpp
 * @brief fish_cam_rpi - on-demand image capture with the OV5647 camera.
 *
 * Execution modes:
 *  - single    : one capture (default, equivalent to --capture).
 *  - burst     : --burst <N> consecutive images.
 *  - timelapse : --timelapse <S> every S seconds (--count <N> to limit).
 *  - demo      : synthetic image generation without a camera (testing only).
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <csignal>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <thread>
#include <vector>

#include "camera/CameraManager.hpp"
#include "config/ConfigManager.hpp"
#include "image/ImageProcessor.hpp"
#include "timestamp/TimestampManager.hpp"
#include "utils/Logger.hpp"

namespace {

volatile std::sig_atomic_t g_stop_requested = 0;

void HandleSignal(int) {
  g_stop_requested = 1;
}

enum class Mode {
  kSingle,
  kBurst,
  kTimelapse,
};

using CliOptions = std::map<std::string, std::string>;

std::string ToLower(std::string text) {
  for (char& c : text) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return text;
}

int ToInt(const std::string& value, int fallback) {
  try {
    return std::stoi(value);
  } catch (const std::exception&) {
    return fallback;
  }
}

bool ParseCli(int argc, char* argv[], CliOptions& opts) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "-h" || arg == "--help") {
      opts["help"] = "true";
    } else if (arg == "--capture") {
      opts["capture"] = "true";
    } else if (arg == "-d" || arg == "--demo") {
      opts["demo"] = "true";
    } else if (arg == "--preview") {
      opts["preview"] = "true";
    } else if (arg == "-n" || arg == "--no-overlay") {
      opts["no-overlay"] = "true";
    } else if (arg == "-v" || arg == "--verbose") {
      opts["verbose"] = "true";
    } else if (arg.rfind("--", 0) == 0) {
      std::string key = arg.substr(2);
      std::string value;
      const std::string::size_type eq = key.find('=');
      if (eq != std::string::npos) {
        value = key.substr(eq + 1);
        key = key.substr(0, eq);
      } else {
        if (i + 1 >= argc) {
          Logger::Error("Missing value for --" + key);
          return false;
        }
        value = std::string(argv[++i]);
      }
      opts[key] = value;
    } else {
      Logger::Error("Unknown argument: " + arg);
      return false;
    }
  }
  return true;
}

void PrintUsage(const char* program) {
  std::cout
      << "fish_cam_rpi - OV5647 camera capture for the Raspberry Pi\n\n"
      << "Usage: " << program << " [options]\n\n"
      << "Capture modes:\n"
      << "  --capture                 Single on-demand capture (default).\n"
      << "  --burst <N>               Burst of N consecutive images.\n"
      << "  --timelapse <S>           Capture every S seconds.\n"
      << "  --count <N>               Limit for timelapse (default: infinite).\n"
      << "  --output <file>           Explicit output filename.\n\n"
      << "Camera settings:\n"
      << "  --config <file>           JSON configuration file (see config/).\n"
      << "  --width <px> / --height <px>\n"
      << "  --fps <n>\n"
      << "  --format <jpg|png>\n"
      << "  --quality <1..100>\n"
      << "  --iso <auto|number>\n"
      << "  --exposure <auto|off|night|backlight|...>\n"
      << "  --rotation <deg>\n"
      << "  --brightness <0..100> / --contrast <0..100>\n"
      << "  --saturation <0..100> / --sharpness <-100..100>\n"
      << "  --preview                 Show a live preview window.\n\n"
      << "Output and logging:\n"
      << "  --output-dir <dir>        Output directory (default ./captures).\n"
      << "  --no-overlay              Do not burn the timestamp into the image.\n"
      << "  --log-file <path>         Log file (default ./logs/camera.log).\n"
      << "  --verbose                 Debug-level logging.\n\n"
      << "Miscellaneous:\n"
      << "  -d, --demo                Demo mode without a camera.\n"
      << "  -h, --help                Show this help and exit.\n";
}

bool ApplyCliOptions(const CliOptions& opts, AppSettings& settings) {
  const auto has = [&opts](const std::string& key) { return opts.count(key) > 0; };
  const auto value = [&opts](const std::string& key) { return opts.at(key); };

  if (has("width")) settings.camera.width = std::max(16, ToInt(value("width"), 1920));
  if (has("height")) settings.camera.height = std::max(16, ToInt(value("height"), 1080));
  if (has("fps")) settings.camera.frame_rate = std::clamp(ToInt(value("fps"), 30), 1, 90);
  if (has("quality")) settings.camera.quality = std::clamp(ToInt(value("quality"), 100), 1, 100);
  if (has("rotation")) settings.camera.rotation = ToInt(value("rotation"), 0);
  if (has("brightness")) settings.camera.brightness = std::clamp(ToInt(value("brightness"), 50), 0, 100);
  if (has("contrast")) settings.camera.contrast = std::clamp(ToInt(value("contrast"), 50), 0, 100);
  if (has("saturation")) settings.camera.saturation = std::clamp(ToInt(value("saturation"), 50), 0, 100);
  if (has("sharpness")) settings.camera.sharpness = std::clamp(ToInt(value("sharpness"), 50), -100, 100);
  if (has("iso")) settings.camera.iso = value("iso");
  if (has("exposure")) settings.camera.exposure = value("exposure");
  if (has("format")) settings.camera.format = ToLower(value("format"));
  if (has("output-dir")) settings.output.directory = value("output-dir");
  if (has("log-file")) settings.logging.file = value("log-file");
  if (has("preview")) settings.camera.show_preview = true;
  if (has("no-overlay")) settings.output.overlay = false;
  if (has("verbose")) settings.logging.level = "debug";

  const std::string format = settings.camera.format;
  if (format != "jpg" && format != "jpeg" && format != "png") {
    Logger::Warning("Unsupported format '" + format + "', falling back to JPG");
    settings.camera.format = "jpg";
  }
  if (settings.camera.format == "jpeg") {
    settings.camera.format = "jpg";
  }
  return true;
}

std::string JoinPath(std::string dir, const std::string& name) {
  if (!dir.empty() && dir.back() != '/') {
    dir += '/';
  }
  return dir + name;
}

bool HasExtension(const std::string& name) {
  const std::string::size_type dot = name.find_last_of('.');
  const std::string::size_type slash = name.find_last_of('/');
  return dot != std::string::npos && (slash == std::string::npos || dot > slash);
}

std::string ResolveUniquePath(const std::string& path, bool overwrite) {
  if (overwrite || !std::filesystem::exists(path)) {
    return path;
  }
  const std::string::size_type dot = path.find_last_of('.');
  const std::string::size_type slash = path.find_last_of('/');
  const bool has_ext = dot != std::string::npos &&
                       (slash == std::string::npos || dot > slash);
  const std::string base = has_ext ? path.substr(0, dot) : path;
  const std::string ext = has_ext ? path.substr(dot) : "";
  for (int index = 1; index < 10000; ++index) {
    const std::string candidate = base + "_" + std::to_string(index) + ext;
    if (!std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return path;
}

std::string BuildOutputPath(const AppSettings& settings,
                            const std::string& explicit_name,
                            const std::string& format) {
  std::string name = explicit_name;
  if (name.empty()) {
    name = TimestampManager::FormatFilename(settings.output.filename_pattern);
  }
  if (!HasExtension(name)) {
    name += "." + format;
  }
  return JoinPath(settings.output.directory, name);
}

void CheckDiskSpace(const std::string& dir, long long min_free_mb = 100) {
  struct statvfs stats {};
  if (::statvfs(dir.c_str(), &stats) != 0) {
    return;
  }
  const long long free_mb =
      (static_cast<long long>(stats.f_bavail) * stats.f_frsize) / (1024LL * 1024LL);
  if (free_mb < min_free_mb) {
    Logger::Warning("Low disk space on " + dir + ": " +
                    std::to_string(free_mb) + " MB free (< " +
                    std::to_string(min_free_mb) + " MB)");
  }
}

std::string FormatSize(double bytes) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(1);
  if (bytes >= 1024.0 * 1024.0) {
    stream << (bytes / (1024.0 * 1024.0)) << " MB";
  } else {
    stream << (bytes / 1024.0) << " KB";
  }
  return stream.str();
}

bool CaptureAndSave(CameraManager& camera, ImageProcessor& processor,
                    const AppSettings& settings,
                    const std::string& explicit_name) {
  const std::string format = settings.camera.format;
  CheckDiskSpace(settings.output.directory);

  const auto capture_start = std::chrono::steady_clock::now();
  cv::Mat frame;
  if (!camera.Capture(frame)) {
    return false;
  }
  const auto capture_end = std::chrono::steady_clock::now();

  if (settings.output.overlay) {
    processor.OverlayTimestamp(frame, TimestampManager::Now());
  }
  const auto process_end = std::chrono::steady_clock::now();

  std::string path = BuildOutputPath(settings, explicit_name, format);
  path = ResolveUniquePath(path, settings.output.overwrite);

  EncodingOptions encoding;
  encoding.jpeg_quality = settings.camera.quality;
  if (!processor.Save(frame, path, encoding)) {
    Logger::Error("Failed to save image: " + path);
    return false;
  }
  const auto save_end = std::chrono::steady_clock::now();

  std::string size_text = "0 KB";
  if (std::filesystem::exists(path)) {
    size_text = FormatSize(static_cast<double>(std::filesystem::file_size(path)));
  }

  const auto capture_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(capture_end - capture_start)
          .count();
  const auto process_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(process_end - capture_end)
          .count();
  const auto total_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(save_end - capture_start)
          .count();

  Logger::Info("Captura exitosa: " + path + " (" + size_text + "), " +
               std::to_string(frame.cols) + "x" + std::to_string(frame.rows) +
               ", captura " + std::to_string(capture_ms) + " ms, overlay " +
               std::to_string(process_ms) + " ms, total " +
               std::to_string(total_ms) + " ms, timestamp: " +
               (settings.output.overlay ? "añadido" : "desactivado"));

  if (settings.output.max_size_mb > 0 &&
      std::filesystem::file_size(path) >
          static_cast<uintmax_t>(settings.output.max_size_mb) * 1024ULL * 1024ULL) {
    Logger::Warning("Image exceeds configured max_size_mb (" +
                    std::to_string(settings.output.max_size_mb) + " MB): " + path);
  }
  return true;
}

int RunDemo(const AppSettings& settings) {
  Logger::Info("Running in DEMO mode (no camera required)");
  ImageProcessor processor;

  cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(18, 44, 96));
  cv::circle(frame, cv::Point(320, 230), 90, cv::Scalar(0, 210, 210), 3, cv::LINE_AA);
  cv::circle(frame, cv::Point(290, 210), 14, cv::Scalar(255, 255, 255), cv::FILLED, cv::LINE_AA);
  cv::circle(frame, cv::Point(350, 210), 14, cv::Scalar(255, 255, 255), cv::FILLED, cv::LINE_AA);
  cv::ellipse(frame, cv::Point(320, 250), cv::Size(26, 14), 0, 0, 180,
              cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
  cv::rectangle(frame, cv::Point(30, 50), cv::Point(610, 430),
                cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

  for (int i = 0; i < 3; ++i) {
    if (settings.output.overlay) {
      processor.OverlayTimestamp(frame, TimestampManager::Now());
    }
    std::string path = BuildOutputPath(settings, "", settings.camera.format);
    path = ResolveUniquePath(path, settings.output.overwrite);
    if (!processor.Save(frame, path, EncodingOptions{})) {
      Logger::Error("Failed to save demo image: " + path);
      return 1;
    }
    Logger::Info("Demo image saved: " + path);
  }
  return 0;
}

void TryRaisePriority() {
  errno = 0;
  if (::setpriority(PRIO_PROCESS, 0, -10) != 0) {
    Logger::Debug("Could not raise process priority: " +
                  std::string(std::strerror(errno)) +
                  " (run with sudo for -10)");
  } else {
    Logger::Info("Process priority raised to -10");
  }
}

void PrintCameraInfo(const CameraManager& camera) {
  const CameraManager::Config config = camera.GetConfig();
  std::cout << "Camera: OV5647 (130-degree wide-angle lens)\n"
            << "Resolution: " << config.width << "x" << config.height << "\n"
            << "Frame rate: " << config.frame_rate << " fps\n"
            << "Encoding: "
            << (config.encoding == CameraManager::Encoding::kPng ? "png" : "jpg")
            << "\n"
            << "Quality: " << config.quality << "\n"
            << "Exposure: " << config.exposure << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  CliOptions opts;
  if (!ParseCli(argc, argv, opts)) {
    return 1;
  }
  if (opts.count("help")) {
    PrintUsage(argv[0]);
    return 0;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  // Load configuration: JSON file (if requested) then CLI overrides.
  AppSettings settings;
  std::string config_path = opts.count("config") ? opts.at("config")
                                                 : "config/camera_config.json";
  if (std::filesystem::exists(config_path)) {
    std::string error;
    if (!ConfigManager::LoadFromFile(config_path, settings, error)) {
      Logger::Error(error);
      Logger::Warning("Continuing with default settings");
      settings = ConfigManager::LoadDefaults();
    } else {
      Logger::Info("Loaded configuration from " + config_path);
    }
  } else {
    settings = ConfigManager::LoadDefaults();
  }
  ApplyCliOptions(opts, settings);

  // Configure logging.
  const std::string::size_type log_slash = settings.logging.file.find_last_of('/');
  if (log_slash != std::string::npos) {
    std::filesystem::create_directories(settings.logging.file.substr(0, log_slash));
  }
  Logger::Instance().Configure(
      settings.logging.file, Logger::ParseLevel(settings.logging.level),
      Logger::Output::kBoth);

  Logger::Info("fish_cam_rpi " FISH_CAM_VERSION " (build date " __DATE__ ")");
  Logger::Info("Config: " + std::to_string(settings.camera.width) + "x" +
               std::to_string(settings.camera.height) + " @" +
               std::to_string(settings.camera.frame_rate) + " fps, " +
               settings.camera.format + " " +
               std::to_string(settings.camera.quality) + "%");
  Logger::Info("Output: " + settings.output.directory + " (pattern " +
               settings.output.filename_pattern + ")");

  // Resolve capture mode.
  Mode mode = Mode::kSingle;
  int burst_count = 1;
  int interval_seconds = 0;
  int max_captures = 0;  // 0 = infinite (timelapse)
  if (opts.count("burst")) {
    mode = Mode::kBurst;
    burst_count = std::max(1, ToInt(opts.at("burst"), 1));
  } else if (opts.count("timelapse")) {
    mode = Mode::kTimelapse;
    interval_seconds = std::max(1, ToInt(opts.at("timelapse"), 60));
    if (opts.count("count")) {
      max_captures = std::max(1, ToInt(opts.at("count"), 0));
    }
  }

  const std::string explicit_name = opts.count("output") ? opts.at("output") : "";

  if (opts.count("demo")) {
    return RunDemo(settings);
  }

  std::filesystem::create_directories(settings.output.directory);
  TryRaisePriority();

  Logger::Info("Detecting cameras...");
  const std::vector<std::string> devices = CameraManager::DetectCameras();
  if (devices.empty()) {
    Logger::Warning(
        "No /dev/video* devices found. The OV5647 may use the legacy MMAL stack.");
  } else {
    for (const auto& device : devices) {
      Logger::Info("Camera detected at " + device);
    }
  }

  CameraManager::Config camera_config;
  camera_config.width = settings.camera.width;
  camera_config.height = settings.camera.height;
  camera_config.frame_rate = settings.camera.frame_rate;
  camera_config.quality = settings.camera.quality;
  camera_config.iso = ToInt(settings.camera.iso, 0);
  camera_config.brightness = settings.camera.brightness;
  camera_config.contrast = settings.camera.contrast;
  camera_config.saturation = settings.camera.saturation;
  camera_config.sharpness = settings.camera.sharpness;
  camera_config.exposure = settings.camera.exposure;
  camera_config.rotation = settings.camera.rotation;
  camera_config.show_preview = settings.camera.show_preview;
  camera_config.encoding =
      (settings.camera.format == "png") ? CameraManager::Encoding::kPng
                                        : CameraManager::Encoding::kJpeg;

  CameraManager camera(camera_config);
  if (!camera.Initialize()) {
    Logger::Critical(
        "Camera initialization failed: " + camera.GetLastError() +
        ". Run script_tools/check_camera.sh and see docs/INSTALL.md.");
    return 1;
  }
  PrintCameraInfo(camera);

  ImageProcessor processor;
  int exit_code = 0;

  if (mode == Mode::kTimelapse) {
    Logger::Info("Timelapse mode: every " + std::to_string(interval_seconds) +
                 " s" + (max_captures > 0
                             ? ", max " + std::to_string(max_captures) + " images"
                             : ", until Ctrl-C"));
    int captured = 0;
    while (!g_stop_requested &&
           (max_captures == 0 || captured < max_captures)) {
      if (CaptureAndSave(camera, processor, settings, "")) {
        ++captured;
      }
      if (!g_stop_requested &&
          (max_captures == 0 || captured < max_captures)) {
        for (int second = 0;
             second < interval_seconds && !g_stop_requested; ++second) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }
    }
  } else if (mode == Mode::kBurst) {
    Logger::Info("Burst mode: capturing " + std::to_string(burst_count) +
                 " images");
    std::vector<cv::Mat> frames;
    const int got = camera.CaptureBurst(burst_count, frames);
    for (int i = 0; i < got; ++i) {
      if (settings.output.overlay) {
        processor.OverlayTimestamp(frames[i], TimestampManager::Now());
      }
      std::string name = explicit_name;
      if (!name.empty() && got > 1) {
        const std::string::size_type dot = name.find_last_of('.');
        const std::string::size_type slash = name.find_last_of('/');
        if (dot != std::string::npos &&
            (slash == std::string::npos || dot > slash)) {
          name = name.substr(0, dot) + "_" + std::to_string(i + 1) +
                 name.substr(dot);
        } else {
          name += "_" + std::to_string(i + 1);
        }
      }
      std::string path = BuildOutputPath(settings, name, settings.camera.format);
      path = ResolveUniquePath(path, settings.output.overwrite);
      if (!processor.Save(frames[i], path, EncodingOptions{})) {
        Logger::Error("Failed to save burst image: " + path);
        exit_code = 1;
        continue;
      }
      Logger::Info("Burst image " + std::to_string(i + 1) + "/" +
                   std::to_string(got) + ": " + path);
    }
  } else {
    Logger::Info("Single capture requested");
    if (!CaptureAndSave(camera, processor, settings, explicit_name)) {
      exit_code = 1;
    }
  }

  camera.Shutdown();
  Logger::Info("fish_cam_rpi terminated (exit " + std::to_string(exit_code) + ")");
  return exit_code;
}
