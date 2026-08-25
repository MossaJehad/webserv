#include "Logger.hpp"
#include <ctime>
#include <iomanip>

LogLevel Logger::_minLevel = LOG_INFO;

void Logger::setLogLevel(LogLevel level) {
    _minLevel = level;
}

void Logger::debug(const std::string& message) {
    log(LOG_DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LOG_INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LOG_WARN, message);
}

void Logger::error(const std::string& message) {
    log(LOG_ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < _minLevel) {
        return;
    }

    std::time_t now = std::time(NULL);
    struct tm* tmInfo = std::localtime(&now);
    char timeBuffer[32];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tmInfo);

    const char* levelStr = "INFO";
    const char* colorCode = "\033[0m";

    switch (level) {
        case LOG_DEBUG:
            levelStr = "DEBUG";
            colorCode = "\033[36m"; // Cyan
            break;
        case LOG_INFO:
            levelStr = "INFO ";
            colorCode = "\033[32m"; // Green
            break;
        case LOG_WARN:
            levelStr = "WARN ";
            colorCode = "\033[33m"; // Yellow
            break;
        case LOG_ERROR:
            levelStr = "ERROR";
            colorCode = "\033[31m"; // Red
            break;
    }

    std::cerr << "[" << timeBuffer << "] [" 
              << colorCode << levelStr << "\033[0m] " 
              << message << std::endl;
}
