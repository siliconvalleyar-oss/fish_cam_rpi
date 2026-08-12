/**
 * @file TimestampManager.hpp
 * @brief Utilities for generating timestamps used in overlays and filenames.
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#ifndef FISH_CAM_TIMESTAMP_TIMESTAMPMANAGER_HPP_
#define FISH_CAM_TIMESTAMP_TIMESTAMPMANAGER_HPP_

#include <string>

namespace fishcam {

/**
 * @brief Provides time formatting helpers.
 *
 * The canonical overlay format is "YYYY-MM-DD HH:MM:SS" and the canonical
 * filename pattern is "fishcam_YYYYMMDD_HHMMSS.<ext>".
 */
class TimestampManager {
 public:
  TimestampManager() = delete;

  /**
   * @brief Returns the current local time formatted with a strftime pattern.
   * @param format strftime-compatible format string.
   * @return The formatted timestamp.
   */
  static std::string Now(const std::string& format = "%Y-%m-%d %H:%M:%S");

  /**
   * @brief Returns a compact stamp for filenames: YYYYMMDD_HHMMSS.
   */
  static std::string FilenameStamp();

  /**
   * @brief Formats a timestamp following a filename pattern.
   * @param pattern strftime pattern (e.g. "fishcam_%Y%m%d_%H%M%S").
   * @param stamp Raw YYYYMMDD_HHMMSS stamp ("" = current time).
   * @return The formatted filename.
   */
  static std::string FormatFilename(const std::string& pattern,
                                    const std::string& stamp = "");

  /**
   * @brief Builds a capture filename with the default prefix and extension.
   * @param extension File extension without the leading dot (e.g. "jpg").
   * @param prefix Custom filename prefix (default "fishcam").
   */
  static std::string BuildFilename(const std::string& extension,
                                   const std::string& prefix = "fishcam");
};

}  // namespace fishcam

#endif  // FISH_CAM_TIMESTAMP_TIMESTAMPMANAGER_HPP_
