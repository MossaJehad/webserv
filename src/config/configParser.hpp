#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "Config.hpp"
#include <string>
#include <vector>

class ConfigParser {
private:
    std::string trim(const std::string& str);
    std::string strip_semicolon(const std::string& str);
    bool        is_valid_method(const std::string& method);

    void process_server_directive(ServerConfig& server, const std::string& key, std::string& line_content);
    void process_location_directive(LocationConfig& location, const std::string& key, std::string& line_content);

public:
    ConfigParser();
    ~ConfigParser();

    std::vector<ServerConfig> parse(const std::string& filepath);
};

#endif