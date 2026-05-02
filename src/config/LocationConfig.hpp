#pragma once
#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    std::vector<std::string> allowedMethods;
    bool autoindex;
    std::string redirectUrl;
    int redirectCode;
    std::string uploadPath;
    std::vector<std::string> cgiExtensions;
    std::string cgiPass;
    long clientMaxBodySize;

    LocationConfig();
    bool isMethodAllowed(const std::string& method) const;
};
