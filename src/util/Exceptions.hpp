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
    // RFC 7231 6.5.5 requires a 405 response to advertise the methods that are
    // permitted on the target resource, so the thrower supplies that list.
    std::string _allow;

public:
    HttpError(int statusCode, const std::string& message,
              const std::string& allow = "")
        : std::runtime_error(message), _statusCode(statusCode), _allow(allow) {}

    // std::exception's destructor is throw(); an explicit match is required in
    // C++98 once the class holds a member with a non-trivial destructor.
    virtual ~HttpError() throw() {}

    int getStatusCode() const {
        return _statusCode;
    }

    const std::string& getAllow() const {
        return _allow;
    }
};

class SystemError : public std::runtime_error {
public:
    explicit SystemError(const std::string& message)
        : std::runtime_error(message) {}
};

#endif
