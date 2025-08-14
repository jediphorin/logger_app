#include "logger.h"

#include <stdexcept>

std::ofstream Logger::logFile;
LogLevel Logger::currentLevel = LogLevel::INFO;
std::mutex Logger::logMutex;

void Logger::init(const std::string &filename, LogLevel defaultLevel) {
  std::lock_guard<std::mutex> lock(logMutex);
  currentLevel = defaultLevel;
  logFile.open(filename, std::ios::out | std::ios::app);

  if (!logFile.is_open()) {
    throw std::runtime_error("не удалось открыть журнал: " + filename);
  }
}

void Logger::setLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(logMutex);
  currentLevel = level;
}

void Logger::log(const std::string &message, LogLevel level) {
  std::lock_guard<std::mutex> lock(logMutex);
  if (level < currentLevel || !logFile.is_open()) {
    return;
  }
  logFile << "[" << getCurrentTime() << "] "
          << "[" << levelStringConverter(level) << "] " << message << std::endl;
}

void Logger::shutdown() {
  std::lock_guard<std::mutex> lock(logMutex);
  if (logFile.is_open()) {
    logFile.close();
  }
}

LogLevel Logger::getCurrentLevel() {
  std::lock_guard<std::mutex> lock(logMutex);
  return currentLevel;
}

std::string Logger::getCurrentLevelString() {
  return levelStringConverter(getCurrentLevel());
}

std::string Logger::levelStringConverter(LogLevel level) {
  switch (level) {
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::ERROR:
    return "ERROR";
  default:
    std::cerr << "неизвестный уровень логирования: " << static_cast<int>(level)
              << std::endl;
    return "UNKNOWN";
  }
}

std::string Logger::levelToString(LogLevel level) {
  try {
    return levelStringConverter(level);
  } catch (...) {
    return "Непонятный уровень логирования(" +
           std::to_string(static_cast<int>(level)) + ").";
  }
}

std::string Logger::getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::tm tm_buffer;

  localtime_r(&in_time_t, &tm_buffer);

  std::stringstream preResult;
  preResult << std::put_time(&tm_buffer, "%Y-%m-%d %H:%M:%S");

  std::string result = preResult.str();
  return result;
}