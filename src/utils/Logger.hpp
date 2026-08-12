/**
 * @file Logger.hpp
 * @brief Thread-safe logging utility with console and file output.
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#ifndef FISH_CAM_UTILS_LOGGER_HPP_
#define FISH_CAM_UTILS_LOGGER_HPP_

#include <fstream>
#include <mutex>
#include <string>

namespace fishcam {

/**
 * @brief Severity levels used by the logger.
 */
enum class LogLevel {
  kDebug = 0,
  kInfo = 1,
  kWarning = 2,
  kError = 3,
  kCritical = 4,
};

/**
 * @brief Thread-safe singleton logger.
 *
 * The logger supports simultaneous output to the console and/or a log file.
 * Every entry is prefixed with a timestamp (YYYY-MM-DD HH:MM:SS) and the
 * severity level. All public methods are safe to call from multiple threads.
 */
class Logger {
 public:
  /** @brief Destination of the log messages. */
  enum class Output {
    kConsole = 0,
    kFile = 1,
    kBoth = 2,
  };

  /**
   * @brief Returns the unique instance of the logger (singleton).
   */
  static Logger& Instance();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  /**
   * @brief Configures the logger.
   * @param log_file Path to the log file (ignored for kConsole output).
   * @param min_level Minimum severity that will be emitted.
   * @param output Destination of the log messages.
   */
  void Configure(const std::string& log_file, LogLevel min_level, Output output);

  /**
   * @brief Emits a message with the given severity.
   * @param level Severity of the message.
   * @param message Text to log.
   */
  void Log(LogLevel level, const std::string& message);

  /** @brief Logs a debug message. */
  static void Debug(const std::string& message);
  /** @brief Logs an informational message. */
  static void Info(const std::string& message);
  /** @brief Logs a warning message. */
  static void Warning(const std::string& message);
  /** @brief Logs an error message. */
  static void Error(const std::string& message);
  /** @brief Logs a critical message. */
  static void Critical(const std::string& message);

  /** @brief Maps a log level name to its enum value (case insensitive). */
  static LogLevel ParseLevel(const std::string& name);

 private:
  Logger() = default;
  ~Logger();

  static const char* LevelToString(LogLevel level);

  std::ofstream file_;
  std::mutex mutex_;
  LogLevel min_level_{LogLevel::kInfo};
  Output output_{Output::kConsole};
  bool file_open_{false};
};

}  // namespace fishcam

#endif  // FISH_CAM_UTILS_LOGGER_HPP_
