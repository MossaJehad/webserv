#include "Time.hpp"
#include <sys/time.h>

std::time_t Time::now() {
    return std::time(NULL);
}

unsigned long Time::nowMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000UL) + (tv.tv_usec / 1000UL);
}

std::string Time::formatHttpDate(std::time_t t) {
    struct tm* gmt = std::gmtime(&t);
    if (!gmt) {
        return "";
    }
    char buf[64];
    // HTTP-date format: "Sun, 06 Nov 1994 08:49:37 GMT"
    if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt) == 0) {
        return "";
    }
    return std::string(buf);
}

std::string Time::currentHttpDate() {
    return formatHttpDate(now());
}
