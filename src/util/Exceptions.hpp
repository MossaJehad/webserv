#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message)
        : std::runtime_error(message) {}
};

class HttpError : public std::runtime_error {
private:
    int _statusCode;

public:
    HttpError(int statusCode, const std::string& message)
        : std::runtime_error(message), _statusCode(statusCode) {}

    int getStatusCode() const {
        return _statusCode;
    }
};

class SystemError : public std::runtime_error {
public:
    explicit SystemError(const std::string& message)
        : std::runtime_error(message) {}
};

#endif
