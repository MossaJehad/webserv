#ifndef TIME_HPP
#define TIME_HPP

#include <ctime>
#include <string>

class Time {
public:
    static std::time_t now();
    static unsigned long nowMs();
    static std::string formatHttpDate(std::time_t t);
    static std::string currentHttpDate();
};

#endif
