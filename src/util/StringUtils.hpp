#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <string>
#include <vector>
#include <sstream>

class StringUtils {
public:
    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);
    static std::vector<std::string> splitWhitespace(const std::string& s);
    static std::string toLower(const std::string& s);
    static std::string toUpper(const std::string& s);
    static bool startsWith(const std::string& s, const std::string& prefix);
    static bool endsWith(const std::string& s, const std::string& suffix);
    static std::string toString(int value);
    static std::string toString(long value);
    static std::string toString(size_t value);
    static int toInt(const std::string& s, int defaultVal = 0);
    static size_t toSizeT(const std::string& s, size_t defaultVal = 0);
    static size_t hexToSizeT(const std::string& s);
    static size_t parseByteSize(const std::string& s);
    static std::string urlDecode(const std::string& s);
    static std::string urlEncode(const std::string& s);
};

#endif
