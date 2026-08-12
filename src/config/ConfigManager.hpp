/**
 * @file ConfigManager.hpp
 * @brief JSON configuration loader (nlohmann/json) for fish_cam_rpi.
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#ifndef FISH_CAM_CONFIG_CONFIGMANAGER_HPP_
#define FISH_CAM_CONFIG_CONFIGMANAGER_HPP_

#include <string>

namespace fishcam {

/** @brief Camera-related settings. */
struct CameraSettings {
  int width = 1920;
  int height = 1080;
  std::string format = "jpg";   ///< "jpg" | "png"
  int quality = 100;            ///< 1..100
  std::string iso = "auto";     ///< "auto" or a numeric ISO value
  int brightness = 50;          ///< 0..100
  int contrast = 50;            ///< 0..100
  int saturation = 50;          ///< 0..100
  int sharpness = 50;           ///< -100..100
  std::string exposure = "auto";///< "auto" | "off" | "night" | ...
  int rotation = 0;             ///< degrees, multiples of 90
  bool horizontal_flip = false;
  bool vertical_flip = false;
  bool show_preview = false;
};

/** @brief Output (storage) settings. */
struct OutputSettings {
  std::string directory = "./captures";
  std::string filename_pattern = "fishcam_%Y%m%d_%H%M%S";
  int max_size_mb = 10;         ///< Size guard (informational/warning).
  bool overwrite = false;       ///< If false, never overwrite existing files.
  bool overlay = true;          ///< Burn the timestamp into the image.
};

/** @brief Logging settings. */
struct LoggingSettings {
  std::string level = "info";   ///< debug | info | warning | error | critical
  std::string file = "./logs/camera.log";
  int max_size_mb = 5;
  int backup_count = 3;
};

/** @brief Complete application settings (top-level config). */
struct AppSettings {
  CameraSettings camera;
  OutputSettings output;
  LoggingSettings logging;
};

/**
 * @brief Loads, validates and serializes the camera_config.json file.
 */
class ConfigManager {
 public:
  ConfigManager() = delete;

  /**
   * @brief Returns a default settings instance (hard-coded defaults).
   */
  static AppSettings LoadDefaults();

  /**
   * @brief Loads settings from a JSON file.
   * @param path Path to the JSON configuration file.
   * @param out Destination for the parsed settings.
   * @param error Human-readable error description on failure.
   * @return true on success.
   */
  static bool LoadFromFile(const std::string& path, AppSettings& out,
                           std::string& error);

  /**
   * @brief Serializes settings to a JSON file.
   * @param path Destination path.
   * @param settings Settings to persist.
   * @return true on success.
   */
  static bool Save(const std::string& path, const AppSettings& settings);
};

}  // namespace fishcam

#endif  // FISH_CAM_CONFIG_CONFIGMANAGER_HPP_
