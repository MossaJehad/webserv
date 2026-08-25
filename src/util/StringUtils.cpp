#include "StringUtils.hpp"
#include <cctype>
#include <cstdlib>
#include <iomanip>

std::string StringUtils::trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start++;
    }
    if (start >= s.size()) {
        return "";
    }
    size_t end = s.size() - 1;
    while (end > start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r' || s[end] == '\n')) {
        end--;
    }
    return s.substr(start, end - start + 1);
}

std::vector<std::string> StringUtils::split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::string current;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) {
            tokens.push_back(current);
            current.clear();
        } else {
            current += s[i];
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::vector<std::string> StringUtils::splitWhitespace(const std::string& s) {
    std::vector<std::string> tokens;
    std::string current;
    for (size_t i = 0; i < s.size(); ++i) {
        if (std::isspace(static_cast<unsigned char>(s[i]))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += s[i];
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string StringUtils::toLower(const std::string& s) {
    std::string res = s;
    for (size_t i = 0; i < res.size(); ++i) {
        res[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(res[i])));
    }
    return res;
}

std::string StringUtils::toUpper(const std::string& s) {
    std::string res = s;
    for (size_t i = 0; i < res.size(); ++i) {
        res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
    }
    return res;
}

bool StringUtils::startsWith(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size()) {
        return false;
    }
    return s.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::endsWith(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) {
        return false;
    }
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string StringUtils::toString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string StringUtils::toString(long value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string StringUtils::toString(size_t value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

int StringUtils::toInt(const std::string& s, int defaultVal) {
    std::string t = trim(s);
    if (t.empty()) {
        return defaultVal;
    }
    char* endPtr = NULL;
    long val = std::strtol(t.c_str(), &endPtr, 10);
    if (*endPtr != '\0') {
        return defaultVal;
    }
    return static_cast<int>(val);
}

size_t StringUtils::toSizeT(const std::string& s, size_t defaultVal) {
    std::string t = trim(s);
    if (t.empty()) {
        return defaultVal;
    }
    char* endPtr = NULL;
    unsigned long val = std::strtoul(t.c_str(), &endPtr, 10);
    if (*endPtr != '\0') {
        return defaultVal;
    }
    return static_cast<size_t>(val);
}

size_t StringUtils::hexToSizeT(const std::string& s) {
    std::string t = trim(s);
    if (t.empty()) {
        return 0;
    }
    char* endPtr = NULL;
    unsigned long val = std::strtoul(t.c_str(), &endPtr, 16);
    if (*endPtr != '\0' && !std::isspace(static_cast<unsigned char>(*endPtr)) && *endPtr != ';') {
        return 0;
    }
    return static_cast<size_t>(val);
}

size_t StringUtils::parseByteSize(const std::string& s) {
    std::string t = trim(s);
    if (t.empty()) {
        return 0;
    }
    char lastChar = t[t.size() - 1];
    size_t multiplier = 1;
    std::string numPart = t;
    if (lastChar == 'k' || lastChar == 'K') {
        multiplier = 1024;
        numPart = t.substr(0, t.size() - 1);
    } else if (lastChar == 'm' || lastChar == 'M') {
        multiplier = 1024 * 1024;
        numPart = t.substr(0, t.size() - 1);
    } else if (lastChar == 'g' || lastChar == 'G') {
        multiplier = 1024 * 1024 * 1024;
        numPart = t.substr(0, t.size() - 1);
    }
    return toSizeT(numPart) * multiplier;
}

static int hexCharToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string StringUtils::urlDecode(const std::string& s) {
    std::string res;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int h1 = hexCharToInt(s[i + 1]);
            int h2 = hexCharToInt(s[i + 2]);
            if (h1 != -1 && h2 != -1) {
                res += static_cast<char>((h1 << 4) | h2);
                i += 2;
                continue;
            }
        } else if (s[i] == '+') {
            res += ' ';
            continue;
        }
        res += s[i];
    }
    return res;
}

std::string StringUtils::urlEncode(const std::string& s) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}
