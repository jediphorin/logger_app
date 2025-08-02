#include "logger.h"
#include <stdexcept>

std::ofstream Logger::logFile;
LogLevel Logger::currentLevel = LogLevel::INFO;
std::mutex Logger::logMutex;

// инициализация логера
void Logger::init(const std::string& filename, LogLevel defaultLevel) {
    
    // захват мьютекса для потокобезопасности
    std::lock_guard<std::mutex> lock(logMutex);

    // установка уровня логирования
    currentLevel = defaultLevel;

    // открытие файла лога
    logFile.open(filename, std::ios::out | std::ios::app);

    // проверка успешности открытия
    if (!logFile.is_open()) {
        // throw std::runtime_error("Failed to open log file: " + filename);
        throw std::runtime_error("не удалось открыть журнал: " + filename);
    }
}

// установка нового уровня логирования
void Logger::setLogLevel(LogLevel level) {

    // захват мьютекса для потокобезопасности
    std::lock_guard<std::mutex> lock(logMutex);

    // установка нового уровня логирования
    currentLevel = level;
}

void Logger::log(const std::string& message, LogLevel level) {
    // захват мьютекса (потокобезопасность)
    std::lock_guard<std::mutex> lock(logMutex);
    
    // уровень должен быть больше или равен текущему, файл должен быть открыт.
    if (level < currentLevel || !logFile.is_open()) {
        return;
    }
    
    // форматированная запись в файл
    logFile << "[" << getCurrentTime() << "] "
            << "[" << getLevelString(level) << "] "
            << message << std::endl;
}

// метод корректного завершения работы логгера
void Logger::shutdown() {

    // захват мьютекса для потокобезопасности
    std::lock_guard<std::mutex> lock(logMutex);

    // если файл открыт, закрываем
    if (logFile.is_open()) {
        logFile.close();
    }
}

LogLevel Logger::getCurrentLevel() {
    std::lock_guard<std::mutex> lock(logMutex);
    return currentLevel;
}

std::string Logger::getCurrentLevelString() {
    std::lock_guard<std::mutex> lock(logMutex);
    return getLevelString(currentLevel);
}

// метод, возвращающий строку, соответствующую уровню логгирования
std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default:
            // Логируем ошибку, если получили некорректное значение
            std::cerr << "неизвестный уровень логирования: " 
                      << static_cast<int>(level) << std::endl;
            return "UNKNOWN";
    }
}

std::string Logger::levelToStringSafe(LogLevel level) {
    try {
        return getLevelString(level);
    } catch (...) {
        return "INVALID_LEVEL(" + std::to_string(static_cast<int>(level)) + ")";
    }
}

// метод получения текущего времени в формате строки
std::string Logger::getCurrentTime() {

    // получение текущего времени
    auto now = std::chrono::system_clock::now();

    // конвертация в t_time
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    // конвертация в локальное время
    std::tm tm_buf;

    // потокобезопасная версия localtime
    localtime_r(&in_time_t, &tm_buf);
    
    // форматирование в строку
    std::stringstream ss;

    // форматирование времени по заданному шаблону
    // ss << std::put_time(&tm_buf, "%Y-%m-%d %X");
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

    // преобразование результата из формата stringstream в string
    std::string result = ss.str();

    // return ss.str();
    return result;
}