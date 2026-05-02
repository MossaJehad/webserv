#include "ServerConfig.hpp"
#include <algorithm>

ServerConfig::ServerConfig()
    : host("0.0.0.0"),
      port(8080),
      clientMaxBodySize(1048576)
{}

const LocationConfig* ServerConfig::matchLocation(const std::string& uri) const
{
    const LocationConfig* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& locPath = locations[i].path;
        if (uri.substr(0, locPath.size()) == locPath) {
            if (locPath.size() > bestLen ||
                (locPath.size() == bestLen && best == NULL)) {
                bestLen = locPath.size();
                best = &locations[i];
            }
        }
    }
    return best;
}

static const std::string EMPTY_STR;

const std::string& ServerConfig::getErrorPage(int code) const
{
    std::map<int, std::string>::const_iterator it = errorPages.find(code);
    if (it != errorPages.end())
        return it->second;
    return EMPTY_STR;
}
