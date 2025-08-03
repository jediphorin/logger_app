#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <memory>
#include <sstream>
#include <iostream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void init(const std::string& filename, LogLevel defaultLevel);
    static void setLogLevel(LogLevel level);
    static void log(const std::string& message, LogLevel level = LogLevel::INFO);
    static void shutdown();
    static LogLevel getCurrentLevel();
    static std::string getCurrentLevelString();
    static std::string levelToStringSafe(LogLevel level);

private:
    static std::ofstream logFile;
    static LogLevel currentLevel;
    static std::mutex logMutex;
    
    static std::string getLevelString(LogLevel level);
    static std::string getCurrentTime();
};

#endif // LOGGER_H