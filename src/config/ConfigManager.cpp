/**
 * @file ConfigManager.cpp
 * @brief Implementation of the JSON configuration loader.
 */

#include "config/ConfigManager.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace fishcam {

namespace {

void ApplyCameraSection(AppSettings& settings, const nlohmann::json& root) {
  if (!root.contains("camera") || !root["camera"].is_object()) {
    return;
  }
  const nlohmann::json& camera = root["camera"];
  if (camera.contains("resolution") && camera["resolution"].is_object()) {
    settings.camera.width = camera["resolution"].value("width", settings.camera.width);
    settings.camera.height = camera["resolution"].value("height", settings.camera.height);
  }
  settings.camera.frame_rate = camera.value("frame_rate", settings.camera.frame_rate);
  settings.camera.format = camera.value("format", settings.camera.format);
  settings.camera.quality = camera.value("quality", settings.camera.quality);
  settings.camera.iso = camera.value("iso", settings.camera.iso);
  settings.camera.brightness = camera.value("brightness", settings.camera.brightness);
  settings.camera.contrast = camera.value("contrast", settings.camera.contrast);
  settings.camera.saturation = camera.value("saturation", settings.camera.saturation);
  settings.camera.sharpness = camera.value("sharpness", settings.camera.sharpness);
  settings.camera.exposure = camera.value("exposure", settings.camera.exposure);
  settings.camera.rotation = camera.value("rotation", settings.camera.rotation);
  settings.camera.horizontal_flip =
      camera.value("horizontal_flip", settings.camera.horizontal_flip);
  settings.camera.vertical_flip =
      camera.value("vertical_flip", settings.camera.vertical_flip);
  settings.camera.show_preview =
      camera.value("show_preview", settings.camera.show_preview);
}

void ApplyOutputSection(AppSettings& settings, const nlohmann::json& root) {
  if (!root.contains("output") || !root["output"].is_object()) {
    return;
  }
  const nlohmann::json& output = root["output"];
  settings.output.directory = output.value("directory", settings.output.directory);
  settings.output.filename_pattern =
      output.value("filename_pattern", settings.output.filename_pattern);
  settings.output.max_size_mb = output.value("max_size_mb", settings.output.max_size_mb);
  settings.output.overwrite = output.value("overwrite", settings.output.overwrite);
  settings.output.overlay = output.value("overlay", settings.output.overlay);
}

void ApplyLoggingSection(AppSettings& settings, const nlohmann::json& root) {
  if (!root.contains("logging") || !root["logging"].is_object()) {
    return;
  }
  const nlohmann::json& logging = root["logging"];
  settings.logging.level = logging.value("level", settings.logging.level);
  settings.logging.file = logging.value("file", settings.logging.file);
  settings.logging.max_size_mb = logging.value("max_size_mb", settings.logging.max_size_mb);
  settings.logging.backup_count = logging.value("backup_count", settings.logging.backup_count);
}

}  // namespace

AppSettings ConfigManager::LoadDefaults() {
  return AppSettings{};
}

bool ConfigManager::LoadFromFile(const std::string& path, AppSettings& out,
                                 std::string& error) {
  std::ifstream stream(path);
  if (!stream.is_open()) {
    error = "Cannot open configuration file: " + path;
    return false;
  }

  nlohmann::json root;
  try {
    stream >> root;
  } catch (const nlohmann::json::exception& ex) {
    error = std::string("Invalid JSON in ") + path + ": " + ex.what();
    return false;
  }

  out = LoadDefaults();
  ApplyCameraSection(out, root);
  ApplyOutputSection(out, root);
  ApplyLoggingSection(out, root);
  return true;
}

bool ConfigManager::Save(const std::string& path, const AppSettings& settings) {
  nlohmann::json root;
  root["camera"]["resolution"]["width"] = settings.camera.width;
  root["camera"]["resolution"]["height"] = settings.camera.height;
  root["camera"]["frame_rate"] = settings.camera.frame_rate;
  root["camera"]["format"] = settings.camera.format;
  root["camera"]["quality"] = settings.camera.quality;
  root["camera"]["iso"] = settings.camera.iso;
  root["camera"]["brightness"] = settings.camera.brightness;
  root["camera"]["contrast"] = settings.camera.contrast;
  root["camera"]["saturation"] = settings.camera.saturation;
  root["camera"]["sharpness"] = settings.camera.sharpness;
  root["camera"]["exposure"] = settings.camera.exposure;
  root["camera"]["rotation"] = settings.camera.rotation;
  root["camera"]["horizontal_flip"] = settings.camera.horizontal_flip;
  root["camera"]["vertical_flip"] = settings.camera.vertical_flip;
  root["camera"]["show_preview"] = settings.camera.show_preview;
  root["output"]["directory"] = settings.output.directory;
  root["output"]["filename_pattern"] = settings.output.filename_pattern;
  root["output"]["max_size_mb"] = settings.output.max_size_mb;
  root["output"]["overwrite"] = settings.output.overwrite;
  root["output"]["overlay"] = settings.output.overlay;
  root["logging"]["level"] = settings.logging.level;
  root["logging"]["file"] = settings.logging.file;
  root["logging"]["max_size_mb"] = settings.logging.max_size_mb;
  root["logging"]["backup_count"] = settings.logging.backup_count;

  std::ofstream stream(path);
  if (!stream.is_open()) {
    return false;
  }
  stream << root.dump(2) << '\n';
  return stream.good();
}

}  // namespace fishcam
