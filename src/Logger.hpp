#ifndef _GENERIC_LOGGER_HPP
#define _GENERIC_LOGGER_HPP

#include "Defines.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <print>
#include <queue>
#include <source_location>
#include <string_view>
#include <thread>

namespace bb::core {

inline constexpr std::string_view COLOR_GREY = "\x1b[90m";
inline constexpr std::string_view COLOR_BLUE = "\x1b[34m";
inline constexpr std::string_view COLOR_GREEN = "\x1b[32m";
inline constexpr std::string_view COLOR_YELLOW = "\x1b[33m";
inline constexpr std::string_view COLOR_RED = "\x1b[31m";
inline constexpr std::string_view COLOR_FATAL = "\x1b[41;97m";
inline constexpr std::string_view COLOR_RESET = "\x1b[0m";

using AtomicBool = std::atomic<bool>;

enum class LogLevel {
  Trace = 0,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
  Off,
};

struct LogMessage {
  LogLevel level;
  std::source_location where;
  string message;
  bool toFile;
};

class Logger {
public:
  inline static Logger &Self() {
    static Logger instance;
    return instance;
  }

  inline void SetLevel(LogLevel lvl) { _level = lvl; }

  inline void SetLogfilePath(const std::filesystem::path &path) {
    _logPath = path;
  }

  inline void EnableFileLogging(bool enable) {
    if (enable) {
      _logFile.open(_logPath, std::ios::out | std::ios::app);
      if (!_logFile.is_open()) {
        Log(LogLevel::Error, std::source_location::current(),
            "Failed to open file");
        _logToFile = false;
        return;
      }
    } else {
      if (_logFile.is_open())
        _logFile.close();
    }
    _logToFile = enable;
  }

  template <typename... Args>
  inline void LogToFile(LogLevel level, std::source_location where,
                        std::string_view fmt, const Args &...args) noexcept {
    if (!_logToFile) {
      Log(LogLevel::Warn, std::source_location::current(),
          "cannot log to file if it was not previously enabled");
      return;
    }

    const bool logToFile = true;
    const string out = format(level, where, fmt, logToFile, args...);
    LogMessage msg{
        .level = level,
        .where = where,
        .message = out,
        .toFile = logToFile,
    };

    std::lock_guard lock(_qMutex);
    pushToQ(msg);
  }

  template <typename... Args>
  void Log(LogLevel level, std::source_location where, std::string_view fmt,
           const Args &...args) noexcept {
    if (level < _level)
      return;

    const bool logToFile = false;
    const string out = format(level, where, fmt, logToFile, args...);
    LogMessage msg{
        .level = level,
        .where = where,
        .message = out,
        .toFile = logToFile,
    };

    std::lock_guard lock(_qMutex);
    pushToQ(msg);
  }

private:
  inline Logger() {
    _workerThread = std::thread([this]() {
      while (_running.load()) {
        std::unique_lock lock(_qMutex);
        _cv.wait(lock, [this]() { return !_logQ.empty() || !_running.load(); });

        while (!_logQ.empty()) {
          LogMessage msg = std::move(_logQ.front());
          _logQ.pop();
          lock.unlock();

          if (msg.toFile && _logToFile && _logFile.is_open()) {
            _logFile << msg.message;
          }

          std::print("{}", msg.message);
          lock.lock();
        }
      }
    });
  }

  ~Logger() {
    _running = false;
    _cv.notify_all();
    if (_workerThread.joinable()) {
      _workerThread.join();
    }

    while (!_logQ.empty()) {
      auto msg = _logQ.front();
      _logQ.pop();
      if (msg.toFile && _logToFile && _logFile.is_open())
        _logFile << msg.message;
      else
        std::print("{}", msg.message);
    }

    if (_logFile.is_open()) {
      _logFile.close();
    }
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  template <typename... Args>
  inline string format(LogLevel level, std::source_location where,
                       std::string_view fmt, bool logToFile,
                       const Args &...args) noexcept {
    auto payload = std::vformat(fmt, std::make_format_args(args...));
    const char *color = LevelColour(level);
    const char *reset = COLOR_RESET.c_str();
    const char *lvlStr = toString(level);
    const char *full = where.file_name();
    const char *p = std::strstr(full, "src/");
    if (!p) {
      auto slash = std::strrchr(full, '/');
      p = slash ? slash + 1 : full;
    }

    if (logToFile) {
      const auto now = std::chrono::system_clock::now();
      std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
      string nowTimeStr = std::ctime(&nowTime);
      std::erase(nowTimeStr, '\n');
      return std::format("[{}] - [{}] {}:{} in function '{}': {}\n", nowTimeStr,
                         lvlStr, p, where.line(), where.function_name(),
                         payload);
    }

    return std::format("{}[{}] {}:{} in function {}'{}'{}: {}{}\n", color,
                       lvlStr, p, where.line(), reset, where.function_name(),
                       color, payload, reset);
  }

  inline void pushToQ(LogMessage msg) {
    _logQ.push(std::move(msg));
    _cv.notify_one();
  }

  inline constexpr const char *toString(LogLevel lvl) const {
    switch (lvl) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Fatal:
      return "FATAL";
    case LogLevel::Off:
      return "";
    }

    // This should never hit
    return "UNKNOWN";
  }

  inline constexpr const char *LevelColour(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::Trace:
      return COLOR_GREY.c_str();
    case LogLevel::Debug:
      return COLOR_BLUE.c_str();
    case LogLevel::Info:
      return COLOR_GREEN.c_str();
      ;
    case LogLevel::Warn:
      return COLOR_YELLOW.c_str();
    case LogLevel::Error:
      return COLOR_RED.c_str();
    case LogLevel::Fatal:
      return COLOR_FATAL.c_str(); // red bg, white text
    default:
      return COLOR_RESET.c_str();
    }
  }

private:
  const std::filesystem::path DEFAULT_PATH = "./log.txt";
  std::fstream _logFile;
  bool _logToFile = false;
  std::filesystem::path _logPath = DEFAULT_PATH;
  std::mutex _mutex;
  LogLevel _level = LogLevel::Trace;

  // Async variables
  std::queue<LogMessage> _logQ;
  std::mutex _qMutex;
  std::condition_variable _cv;
  std::thread _workerThread;
  AtomicBool _running = true;
};

} // namespace bb::core

// —————— macros ——————
#ifdef NDEBUG
#define LOG_TRACE(...)                                                         \
  do {                                                                         \
  } while (0)
#define LOG_DEBUG(...)                                                         \
  do {                                                                         \
  } while (0)
#define LOG_INFO(...)                                                          \
  do {                                                                         \
  } while (0)
#define LOG_WARN(...)                                                          \
  do {                                                                         \
  } while (0)
#define FLOG_TRACE(...)                                                        \
  do {                                                                         \
  } while (0)
#define FLOG_DEBUG(...)                                                        \
  do {                                                                         \
  } while (0)
#define FLOG_INFO(...)                                                         \
  do {                                                                         \
  } while (0)
#define FLOG_WARN(...)                                                         \
  do {                                                                         \
  } while (0)
#else
#define LOG_TRACE(fmt, ...)                                                    \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Trace,                      \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                    \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Debug,                      \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Info,                       \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Warn,                       \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)

#define FLOG_TRACE(fmt, ...)                                                   \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Trace,                \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)
#define FLOG_DEBUG(fmt, ...)                                                   \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Debug,                \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)
#define FLOG_INFO(fmt, ...)                                                    \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Info,                 \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)
#define FLOG_WARN(fmt, ...)                                                    \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Warn,                 \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)

#endif

#define LOG_ERROR(fmt, ...)                                                    \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Error,                      \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)                                                    \
  bb::core::Logger::Self().Log(bb::core::LogLevel::Fatal,                      \
                               std::source_location::current(), fmt,           \
                               ##__VA_ARGS__)

#define FLOG_ERROR(fmt, ...)                                                   \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Error,                \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)
#define FLOG_FATAL(fmt, ...)                                                   \
  bb::core::Logger::Self().LogToFile(bb::core::LogLevel::Fatal,                \
                                     std::source_location::current(), fmt,     \
                                     ##__VA_ARGS__)

#define ENABLE_FILE_LOGGING(enable)                                            \
  bb::core::Logger::Self().EnableFileLogging(enable);

#define SET_LOG_FILE(filepath)                                                 \
  bb::core::Logger::Self().SetLogfilePath(filepath);

#endif // _GENERIC_LOGGER_HPP
