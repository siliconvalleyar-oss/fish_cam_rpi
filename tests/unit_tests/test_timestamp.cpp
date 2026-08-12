/**
 * @file test_timestamp.cpp
 * @brief Unit tests for the TimestampManager class.
 *
 * Build and run with: make test
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#include <cassert>
#include <iostream>
#include <string>
#include <regex>

#include "timestamp/TimestampManager.hpp"

namespace {

void TestOverlayFormat() {
  const std::string stamp = fishcam::TimestampManager::Now();
  std::regex pattern(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");
  assert(std::regex_match(stamp, pattern));
  std::cout << "  [OK] overlay format matches YYYY-MM-DD HH:MM:SS\n";
}

void TestFilenameStamp() {
  const std::string stamp = fishcam::TimestampManager::FilenameStamp();
  std::regex pattern(R"(\d{8}_\d{6})");
  assert(std::regex_match(stamp, pattern));
  std::cout << "  [OK] filename stamp matches YYYYMMDD_HHMMSS\n";
}

void TestBuildFilename() {
  const std::string name = fishcam::TimestampManager::BuildFilename("jpg");
  std::regex pattern(R"(fishcam_\d{8}_\d{6}\.jpg)");
  assert(std::regex_match(name, pattern));
  std::cout << "  [OK] BuildFilename -> " << name << "\n";
}

void TestFormatFilename() {
  const std::string name = fishcam::TimestampManager::FormatFilename(
      "fishcam_%Y%m%d_%H%M%S");
  std::regex pattern(R"(fishcam_\d{8}_\d{6})");
  assert(std::regex_match(name, pattern));
  std::cout << "  [OK] FormatFilename -> " << name << "\n";
}

}  // namespace

int main() {
  std::cout << "TimestampManager tests\n";
  TestOverlayFormat();
  TestFilenameStamp();
  TestBuildFilename();
  TestFormatFilename();
  std::cout << "All timestamp tests passed.\n";
  return 0;
}
