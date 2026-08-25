#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include "ServerConfig.hpp"
#include <vector>

class ConfigValidator {
public:
    static void validate(std::vector<ServerConfig>& servers);
};

#endif
