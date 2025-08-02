#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "logger.h"

struct LogTask {
    std::string message;
    LogLevel level;
};

class LogWorker {
public:
    LogWorker() : running(true) {
        workerThread = std::thread(&LogWorker::process, this);
    }
    
    ~LogWorker() {
        running = false;
        cv.notify_one();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
    
    void addTask(const LogTask& task) {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.push(task);
        cv.notify_one();
    }
    
private:
    std::queue<LogTask> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::thread workerThread;
    std::atomic<bool> running;
    
    void process() {
        while (running) {
            LogTask task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this]() { return !tasks.empty() || !running; });
                
                if (!running && tasks.empty()) {
                    return;
                }
                
                if (!tasks.empty()) {
                    task = tasks.front();
                    tasks.pop();
                }
            }
            
            if (!task.message.empty()) {
                try {
                    Logger::log(task.message, task.level);
                } catch (const std::exception& e) {
                    std::cerr << "Logging error: " << e.what() << std::endl;
                }
            }
        }
    }
};

// LogLevel parseLogLevel(const std::string& levelStr, LogLevel defaultLevel) {
LogLevel parseLogLevel(const std::string& levelStr) {
    if (levelStr == "DEBUG") return LogLevel::DEBUG;
    if (levelStr == "INFO") return LogLevel::INFO;
    if (levelStr == "WARNING") return LogLevel::WARNING;
    if (levelStr == "ERROR") return LogLevel::ERROR;
    throw std::invalid_argument("некорректный уровень логирования: " + levelStr);
    // return defaultLevel;
    // return LogLevel::INFO; // По умолчанию
}

void printHelp() {
    std::cout << "Доступные команды:\n"
              << "  message [level] - уровень логирования (DEBUG, INFO, WARNING, ERROR)\n"
              << "  getlevel - узнать текущий уровень логирования\n"
              << "  setlevel LEVEL - изменение уровня логирования\n"
              << "  help - показать эту помощь\n"
              << "  exit - выйти из приложения\n";
}

int main(int argc, char* argv[]) {

    if (argc != 3) { 
        std::cerr << "Использование: " << argv[0] << " <файл_журнала> <уровень>\n"
                  << "Уровни: DEBUG, INFO, WARNING, ERROR\n";
        return 1;
    }
    
    std::string logFile = argv[1];
    try {
        LogLevel defaultLevel = parseLogLevel(argv[2]);
        Logger::init(logFile, defaultLevel);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        std::cerr << "Допустимые уровни: DEBUG, INFO, WARNING, ERROR." << std::endl;
        return 1;
    }
    
    LogWorker worker;
    std::cout << "Логер инициализирован. Напечатай 'help' для обзора команд.\n";
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        if (input == "exit") {
            break;
        }
        
        if (input == "help") {
            printHelp();
            continue;
        }

        if (input.rfind("getlevel", 0) == 0) {
            std::string currentLevelStr = Logger::getCurrentLevelString();
            std::cout << "уровень логирования: " << currentLevelStr << std::endl;
        }
        
        if (input.rfind("setlevel ", 0) == 0) {

            try {
                std::string levelStr = input.substr(9);
                LogLevel newLevel = parseLogLevel(levelStr);
                Logger::setLogLevel(newLevel);
                std::cout << "уровень логирования установлен: " << levelStr << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "ошибка: " << e.what() << std::endl;
            }
            continue;
        }
        
        if (input.rfind("message ", 0) == 0) {
            std::string rest = input.substr(8);
            size_t spacePos = rest.find(' ');            

            LogTask task;
            if (spacePos != std::string::npos) {
                // если уровень не указан
                std::string levelStr = rest.substr(0, spacePos);
                try {
                    task.level = parseLogLevel(levelStr);
                    task.message = rest.substr(spacePos + 1);
                } catch (...) {
                    // если уровень не корректен, то используем текущий
                    task.level = Logger::getCurrentLevel();
                    // всё сообщение
                    task.message = rest;
                }
            } else {
                // если уровень не указан, то используем текущий
                task.level = Logger::getCurrentLevel();
                task.message = rest;                
            }

            worker.addTask(task);
            std::cout << "Сообщение добавлено (уровень = "
                      << Logger::levelToStringSafe(task.level)
                      << ")" << std::endl;
        }
        
        std::cout << "Неизвестная команда. Напечатай 'help' для обзора команд.\n";
    }
    
    Logger::shutdown();
    return 0;
}