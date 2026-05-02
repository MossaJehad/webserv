#include "LocationConfig.hpp"
#include <algorithm>

LocationConfig::LocationConfig()
    : autoindex(false),
      redirectCode(0),
      clientMaxBodySize(1048576)
{
    allowedMethods.push_back("GET");
    allowedMethods.push_back("POST");
    allowedMethods.push_back("DELETE");
}

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
    for (size_t i = 0; i < allowedMethods.size(); ++i) {
        if (allowedMethods[i] == method)
            return true;
    }
    return false;
}
