#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "logger.h"
#include <cctype>

//  структура из сообщения и уровеня логирования для занесения в журнал
struct LogTask {
    std::string message;
    LogLevel level;
};

// выполняет запись логов в фоновом режиме, пока основная программа продолжает работу
// принимает сообщения из разных потоков и гарантирует их корректную обработку
// автоматически запускает и останавливает рабочий поток
class LogWorker {
public:
    // конструктор запускает поток, который выполняет метод process()
    LogWorker() : running(true) {
        workerThread = std::thread(&LogWorker::process, this);
    }
    
    // деструктор
    ~LogWorker() {
        // запрос на остановку потока
        running = false;

        // будим поток, если он спит
        cv.notify_one();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
    
    void addTask(const LogTask& task) {
        std::lock_guard<std::mutex> lock(queueMutex);

        // добавление задачи в очередь
        tasks.push(task);

        // будим поток-обработчик
        cv.notify_one();
    }
    
private:
    // очередь задач
    std::queue<LogTask> tasks;
    // защищает очередь от гонки задач
    std::mutex queueMutex;
    // сигнализирует о новых задачах
    std::condition_variable cv;
    // фоновый поток-обработчик
    std::thread workerThread;
    // флаг работы потока
    std::atomic<bool> running;
    
    // основной цикл обработки
    void process() {
        while (running) {
            LogTask task;
            {
                // ожидаем задач или остановки
                std::unique_lock<std::mutex> lock(queueMutex);

                // блокирует поток, пока нет задачи
                cv.wait(lock, [this]() { return !tasks.empty() || !running; });

                if (!running) {
                    while (!tasks.empty()) {
                        task = tasks.front();
                        tasks.pop();
                        lock.unlock();
                        Logger::log(task.message, task.level);
                        lock.lock();
                    }
                    return;
                }
                task = tasks.front();
                tasks.pop();
                
                /*if (!running && tasks.empty()) {
                    return;
                }
                
                // извлекаем задачу
                if (!tasks.empty()) {
                    task = tasks.front();
                    tasks.pop();
                }*/
            }
            Logger::log(task.message, task.level);
            // записываем в лог (уже без блокировки)
            /*if (!task.message.empty()) {
                try {
                    Logger::log(task.message, task.level);
                } catch (const std::exception& e) {
                    std::cerr << "ошибка логирования: " << e.what() << std::endl;
                }
            }*/
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
              << "  message [LEVEL] - уровень логирования (DEBUG, INFO, WARNING, ERROR)\n"
              << "  getlevel - узнать текущий уровень логирования\n"
              << "  setlevel [LEVEL] - изменение уровня логирования\n"
              << "  help - показать эту помощь\n"
              << "  exit - выйти из приложения\n";
}

std::string trimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    std::string result;
    if (start == std::string::npos) {
        result = "";
    } else {
        result = str.substr(start);
    }
    return result;
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
    
    std::string preInput;
    std::string input;
    while (true) {
        std::cout << "> ";

        // std::getline(std::cin, input);
        std::getline(std::cin, preInput);
        input = trimLeft(preInput);

        if (input.empty()) {
            continue;
        }        
        else if (input == "exit") {
            break;
        }        
        else if (input == "help") {
            printHelp();
            continue;
        }
        else if (input.rfind("getlevel", 0) == 0) {
            std::cout   << "уровень логирования: " 
                        << Logger::getCurrentLevelString() 
                        << std::endl;
        }        
        else if (input.rfind("setlevel ", 0) == 0) {

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
        else if (input.rfind("message ", 0) == 0) {
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
        else {
            std::cout << "Неизвестная команда. Напечатай 'help' для обзора команд." << std::endl;
        }       
        // std::cout << "Напечатай 'help' для обзора команд.\n";
    }
    Logger::shutdown();

    return 0;
}