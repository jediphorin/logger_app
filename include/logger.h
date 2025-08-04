#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel { DEBUG, INFO, ERROR };

class Logger {
 public:
  static void init(const std::string& filename, LogLevel defaultLevel);
  static void setLogLevel(LogLevel level);
  static void log(const std::string& message, LogLevel level = LogLevel::INFO);
  static void shutdown();
  static LogLevel getCurrentLevel();
  static std::string getCurrentLevelString();
  static std::string levelToString(LogLevel level);

 private:
  static std::ofstream logFile;
  static LogLevel currentLevel;
  static std::mutex logMutex;

  static std::string levelStringConverter(LogLevel level);
  static std::string getCurrentTime();
};

#endif  // LOGGER_H