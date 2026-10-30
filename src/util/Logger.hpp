#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

class Logger {
private:
    static LogLevel _minLevel;

    Logger(); // Disallow instantiation

public:
    static void setLogLevel(LogLevel level);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void log(LogLevel level, const std::string& message);
};

#endif
