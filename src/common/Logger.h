#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace arenanet {
namespace common {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        std::cout << "[" << std::put_time(std::localtime(&now_c), "%F %T") << "] ";

        switch (level) {
            case LogLevel::INFO:
                std::cout << "[INFO] ";
                break;
            case LogLevel::WARNING:
                std::cout << "[WARNING] ";
                break;
            case LogLevel::ERROR:
                std::cout << "[ERROR] ";
                break;
            case LogLevel::DEBUG:
                std::cout << "[DEBUG] ";
                break;
        }

        std::cout << message << std::endl;
    }

    static void info(const std::string& msg) { getInstance().log(LogLevel::INFO, msg); }
    static void warning(const std::string& msg) { getInstance().log(LogLevel::WARNING, msg); }
    static void error(const std::string& msg) { getInstance().log(LogLevel::ERROR, msg); }
    static void debug(const std::string& msg) { getInstance().log(LogLevel::DEBUG, msg); }

private:
    Logger() = default;
    std::mutex mutex_;
};

#define LOG_INFO(msg) arenanet::common::Logger::info(msg)
#define LOG_WARN(msg) arenanet::common::Logger::warning(msg)
#define LOG_ERROR(msg) arenanet::common::Logger::error(msg)
#define LOG_DEBUG(msg) arenanet::common::Logger::debug(msg)

} // namespace common
} // namespace arenanet
