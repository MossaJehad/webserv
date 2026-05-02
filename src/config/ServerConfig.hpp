#pragma once
#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

struct ServerConfig {
    std::string host;
    int port;
    std::vector<std::string> serverNames;
    std::map<int, std::string> errorPages;
    long clientMaxBodySize;
    std::vector<LocationConfig> locations;

    ServerConfig();
    const LocationConfig* matchLocation(const std::string& uri) const;
    const std::string& getErrorPage(int code) const;
};
