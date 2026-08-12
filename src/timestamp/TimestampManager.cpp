/**
 * @file TimestampManager.cpp
 * @brief Implementation of timestamp helpers.
 */

#include "timestamp/TimestampManager.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace fishcam {

std::string TimestampManager::Now(const std::string& format) {
  const std::time_t now = std::time(nullptr);
  std::tm tm_buffer{};
  localtime_r(&now, &tm_buffer);
  std::ostringstream stream;
  stream << std::put_time(&tm_buffer, format.c_str());
  return stream.str();
}

std::string TimestampManager::FilenameStamp() {
  return Now("%Y%m%d_%H%M%S");
}

std::string TimestampManager::FormatFilename(const std::string& pattern,
                                             const std::string& stamp) {
  if (stamp.empty()) {
    return Now(pattern);
  }
  // Rebuild a struct tm from the YYYYMMDD_HHMMSS stamp.
  std::tm tm_buffer{};
  std::istringstream in(stamp);
  in >> std::get_time(&tm_buffer, "%Y%m%d_%H%M%S");
  std::ostringstream out;
  out << std::put_time(&tm_buffer, pattern.c_str());
  return out.str();
}

std::string TimestampManager::BuildFilename(const std::string& extension,
                                            const std::string& prefix) {
  return prefix + "_" + FilenameStamp() + "." + extension;
}

}  // namespace fishcam
