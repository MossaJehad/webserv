#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ConfigTokenizer.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <vector>

class ConfigParser {
private:
    std::vector<Token> _tokens;
    size_t _pos;

    const Token& peek() const;
    const Token& current() const;
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& errorMsg);
    void expectSemicolon();

    void parseServer(std::vector<ServerConfig>& servers);
    void parseServerDirective(ServerConfig& server);
    void parseLocation(ServerConfig& server);
    void parseLocationDirective(LocationConfig& location);

    void parseListen(ServerConfig& server);
    void parseServerName(ServerConfig& server);
    void parseErrorPage(ServerConfig& server);
    void parseClientMaxBodySize(ServerConfig& server);
    void parseServerRoot(ServerConfig& server);
    void parseServerIndex(ServerConfig& server);

    void parseLocationRoot(LocationConfig& location);
    void parseLocationIndex(LocationConfig& location);
    void parseLocationAutoindex(LocationConfig& location);
    void parseLocationMethods(LocationConfig& location);
    void parseLocationReturn(LocationConfig& location);
    void parseLocationUploadDir(LocationConfig& location);
    void parseLocationCgi(LocationConfig& location);
    void parseLocationBodySize(LocationConfig& location);

public:
    ConfigParser();
    ~ConfigParser();

    std::vector<ServerConfig> parse(const std::string& configFilePath);
    std::vector<ServerConfig> parseContent(const std::string& content);
};

#endif
