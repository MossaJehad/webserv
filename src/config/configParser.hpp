#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "Config.hpp"
#include <string>
#include <vector>

class ConfigParser {
private:
    std::string              trim(const std::string& str);
    std::string              strip_semicolon(const std::string& str);
    std::vector<std::string> preprocess_file(const std::string& filepath);
    bool                     is_valid_method(const std::string& method);
    bool                     is_valid_host(const std::string& host);
    bool                     is_valid_client_max_body_size(const std::string& size_str);
    
    void process_server_directive(ServerConfig& server, const std::string& key, std::string& line_content);
    void process_location_directive(LocationConfig& location, const std::string& key, std::string& line_content);

public:
    ConfigParser();
    ~ConfigParser();

    void                      save_preprocessed_file(const std::string& output_filepath, const std::vector<std::string>& virtual_lines);
    std::vector<std::string>  get_preprocessed_lines(const std::string& filepath) { return preprocess_file(filepath); }
    std::vector<ServerConfig> parse(const std::string& filepath);
};

#endif // CONFIG_PARSER_HPP