/**
 * @file Logger.cpp
 * @brief Implementation of the Logger utility.
 */

#include "utils/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fishcam {

Logger& Logger::Instance() {
  static Logger instance;
  return instance;
}

Logger::~Logger() {
  if (file_.is_open()) {
    file_.close();
  }
}

void Logger::Configure(const std::string& log_file, LogLevel min_level,
                       Output output) {
  std::lock_guard<std::mutex> lock(mutex_);
  min_level_ = min_level;
  output_ = output;
  if (!log_file.empty() && (output == Output::kFile || output == Output::kBoth)) {
    file_.open(log_file, std::ios::app);
    file_open_ = file_.is_open();
    if (!file_open_) {
      std::cerr << "[WARN] Unable to open log file: " << log_file << '\n';
    }
  }
}

void Logger::Log(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (level < min_level_) {
    return;
  }

  const std::time_t now = std::time(nullptr);
  std::tm tm_buffer{};
  localtime_r(&now, &tm_buffer);

  std::ostringstream entry;
  entry << '[' << std::put_time(&tm_buffer, "%Y-%m-%d %H:%M:%S") << "] ["
        << LevelToString(level) << "] " << message;

  if (output_ == Output::kConsole || output_ == Output::kBoth) {
    std::cout << entry.str() << '\n';
  }
  if (file_open_ && (output_ == Output::kFile || output_ == Output::kBoth)) {
    file_ << entry.str() << '\n';
    file_.flush();
  }
}

void Logger::Debug(const std::string& message) {
  Instance().Log(LogLevel::kDebug, message);
}

void Logger::Info(const std::string& message) {
  Instance().Log(LogLevel::kInfo, message);
}

void Logger::Warning(const std::string& message) {
  Instance().Log(LogLevel::kWarning, message);
}

void Logger::Error(const std::string& message) {
  Instance().Log(LogLevel::kError, message);
}

void Logger::Critical(const std::string& message) {
  Instance().Log(LogLevel::kCritical, message);
}

LogLevel Logger::ParseLevel(const std::string& name) {
  std::string lowered = name;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (lowered == "debug") return LogLevel::kDebug;
  if (lowered == "info") return LogLevel::kInfo;
  if (lowered == "warning" || lowered == "warn") return LogLevel::kWarning;
  if (lowered == "error") return LogLevel::kError;
  if (lowered == "critical") return LogLevel::kCritical;
  return LogLevel::kInfo;
}

const char* Logger::LevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarning:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
    case LogLevel::kCritical:
      return "CRITICAL";
  }
  return "UNKNOWN";
}

}  // namespace fishcam
