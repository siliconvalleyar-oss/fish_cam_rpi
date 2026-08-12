/**
 * @file test_config.cpp
 * @brief Unit tests for the ConfigManager class (JSON round-trip).
 *
 * Build and run with: make test
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "config/ConfigManager.hpp"

namespace {

void TestDefaults() {
  const fishcam::AppSettings settings = fishcam::ConfigManager::LoadDefaults();
  assert(settings.camera.width == 1920);
  assert(settings.camera.height == 1080);
  assert(settings.camera.quality == 100);
  assert(settings.camera.format == "jpg");
  assert(settings.output.directory == "./captures");
  assert(settings.output.overlay);
  assert(settings.logging.level == "info");
  std::cout << "  [OK] defaults\n";
}

void TestRoundTrip() {
  const std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") +
                           "/fish_cam_test_config.json";

  fishcam::AppSettings settings = fishcam::ConfigManager::LoadDefaults();
  settings.camera.width = 1280;
  settings.camera.height = 720;
  settings.camera.format = "png";
  settings.output.overlay = false;
  assert(fishcam::ConfigManager::Save(path, settings));

  fishcam::AppSettings loaded;
  std::string error;
  assert(fishcam::ConfigManager::LoadFromFile(path, loaded, error));
  assert(loaded.camera.width == 1280);
  assert(loaded.camera.height == 720);
  assert(loaded.camera.format == "png");
  assert(!loaded.output.overlay);
  std::cout << "  [OK] JSON round-trip preserved values\n";

  std::filesystem::remove(path);
}

void TestInvalidJson() {
  const std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") +
                           "/fish_cam_bad_config.json";
  std::ofstream stream(path);
  stream << "{ not valid json !!!\n";
  stream.close();

  fishcam::AppSettings settings;
  std::string error;
  assert(!fishcam::ConfigManager::LoadFromFile(path, settings, error));
  assert(!error.empty());
  std::cout << "  [OK] invalid JSON rejected (" << error << ")\n";

  std::filesystem::remove(path);
}

}  // namespace

int main() {
  std::cout << "ConfigManager tests\n";
  TestDefaults();
  TestRoundTrip();
  TestInvalidJson();
  std::cout << "All config tests passed.\n";
  return 0;
}
