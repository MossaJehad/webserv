#pragma once
#include <string>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    bool parse(const std::string& filename);
    const std::vector<ServerConfig>& getServers() const;
    const std::string& getError() const;

private:
    std::vector<ServerConfig> _servers;
    std::string _error;

    std::vector<std::string> _tokens;
    size_t _pos;

    bool tokenize(const std::string& filename);
    std::string nextToken();
    std::string peekToken() const;
    bool expect(const std::string& tok);

    bool parseServer();
    bool parseLocation(ServerConfig& srv);
    bool parseErrorPage(ServerConfig& srv);
    long parseSize(const std::string& val);
};
