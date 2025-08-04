#include <atomic>
#include <cctype>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

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
    condition.notify_one();
    if (workerThread.joinable()) {
      workerThread.join();
    }
  }

  void addTask(const LogTask& task) {
    std::lock_guard<std::mutex> lock(queueMutex);
    tasks.push(task);
    condition.notify_one();
  }

  void printInLog(LogTask task) {
    try {
      Logger::log(task.message, task.level);
    } catch (const std::ios_base::failure& e) {
      std::cerr << "ошибка записи в лог: " << e.what() << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "обожемой! Какая-то ошибка: " << e.what() << std::endl;
    }
  }

 private:
  std::queue<LogTask> tasks;
  std::mutex queueMutex;
  std::condition_variable condition;
  std::thread workerThread;
  std::atomic<bool> running;

  void process() {
    while (running) {
      LogTask task;
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        while (tasks.empty() && running) {
          condition.wait(lock);
        }
        if (!running) {
          while (!tasks.empty()) {
            task = tasks.front();
            tasks.pop();
            lock.unlock();
            printInLog(task);
            lock.lock();
          }
          return;
        }
        task = tasks.front();
        tasks.pop();
      }
      printInLog(task);
    }
  }
};

LogLevel parseLogLevel(const std::string& levelStr) {
  if (levelStr == "DEBUG")
    return LogLevel::DEBUG;
  else if (levelStr == "INFO")
    return LogLevel::INFO;
  else if (levelStr == "ERROR")
    return LogLevel::ERROR;
  throw std::invalid_argument("некорректный уровень логирования: " + levelStr);
}

void printHelp() {
  std::cout << "Доступные команды:\n"
            << "  message [LEVEL] - уровень логирования (DEBUG, INFO, ERROR)\n"
            << "  getlevel - узнать текущий уровень логирования\n"
            << "  setlevel [LEVEL] - изменение уровня логирования\n"
            << "  help - показать эту помощь\n"
            << "  exit - выйти из приложения\n";
}

void trimLeft(std::string& str) {
  auto start = str.find_first_not_of(" \t\n\r\f\v\u00A0");
  if (start != std::string::npos) {
    str.erase(0, start);
  } else {
    str.clear();
  }
}

void trimRight(std::string& str) {
  auto end = str.find_last_not_of(" \t\n\r\f\v\u00A0");
  if (end != std::string::npos) {
    str.resize(end + 1);
  } else {
    str.clear();
  }
}

std::string fullTrim(const std::string& str) {
  std::string trimed = str;
  trimLeft(trimed);
  trimRight(trimed);
  return trimed;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Использование: " << argv[0] << " <файл_журнала> <уровень>\n"
              << "Уровни: DEBUG, INFO, ERROR\n";
    return 1;
  }
  std::string logFile = argv[1];
  try {
    LogLevel defaultLevel = parseLogLevel(argv[2]);
    Logger::init(logFile, defaultLevel);
  } catch (const std::exception& e) {
    std::cerr << "Ошибка: " << e.what() << std::endl;
    std::cerr << "Допустимые уровни: DEBUG, INFO, ERROR." << std::endl;
    return 1;
  }
  LogWorker worker;
  std::cout << "Логер инициализирован. Напечатай 'help' для обзора команд.\n";

  std::string preInput;
  std::string input;
  while (true) {
    std::cout << "> ";
    std::getline(std::cin, input);
    trimLeft(input);
    trimRight(input);
    if (input.empty()) {
      continue;
    } else if (input == "exit") {
      break;
    } else if (input == "help") {
      printHelp();
      continue;
    } else if (input == "getlevel") {
      std::cout << "уровень логирования: " << Logger::getCurrentLevelString()
                << std::endl;
    } else if (input.rfind("setlevel", 0) == 0) {
      try {
        std::string preLevelStr = input.substr(8);
        std::string levelStr = fullTrim(preLevelStr);

        if (levelStr.empty()) {
          throw std::invalid_argument("Не указан уровень логирования.");
        }
        LogLevel newLevel = parseLogLevel(levelStr);
        Logger::setLogLevel(newLevel);
        std::cout << "уровень логирования установлен: " << levelStr
                  << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "ошибка: " << e.what() << std::endl;
      }
      continue;
    } else if (input.find("message") != std::string::npos) {
      size_t msg_pos = input.find("message");
      std::string otherPartOfInput = input.substr(msg_pos + 7);
      std::replace(otherPartOfInput.begin(), otherPartOfInput.end(), '\t', ' ');

      trimLeft(otherPartOfInput);
      size_t firstSpacePosition = otherPartOfInput.find(' ');

      LogTask task;
      if (firstSpacePosition != std::string::npos) {
        std::string levelStr = otherPartOfInput.substr(0, firstSpacePosition);
        trimRight(levelStr);

        try {
          task.level = parseLogLevel(levelStr);
          task.message = otherPartOfInput.substr(firstSpacePosition + 1);
          trimLeft(task.message);
        } catch (...) {
          task.level = Logger::getCurrentLevel();
          task.message = otherPartOfInput;
        }
      } else {
        task.level = Logger::getCurrentLevel();
        task.message = otherPartOfInput;
      }
      worker.addTask(task);
      if (task.level < Logger::getCurrentLevel()) {
        std::cout << "попытка добавить сообщение (уровень = ";
      } else {
        std::cout << "добавлено сообщение (уровень = ";
      }
      std::cout << Logger::levelToString(task.level) << ")" << std::endl;
    } else {
      std::cout << "Неизвестная команда. Напечатай 'help' для обзора команд."
                << std::endl;
    }
  }
  Logger::shutdown();
  return 0;
}