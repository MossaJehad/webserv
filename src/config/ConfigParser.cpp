#include "ConfigParser.hpp"
#include "Exceptions.hpp"
#include "StringUtils.hpp"
#include <sstream>

ConfigParser::ConfigParser() : _pos(0) {}
ConfigParser::~ConfigParser() {}

const Token& ConfigParser::peek() const {
    if (_pos + 1 < _tokens.size()) {
        return _tokens[_pos + 1];
    }
    return _tokens.back();
}

const Token& ConfigParser::current() const {
    if (_pos < _tokens.size()) {
        return _tokens[_pos];
    }
    return _tokens.back();
}

Token ConfigParser::advance() {
    if (_pos < _tokens.size()) {
        return _tokens[_pos++];
    }
    return _tokens.back();
}

bool ConfigParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool ConfigParser::check(TokenType type) const {
    return current().type == type;
}

Token ConfigParser::consume(TokenType type, const std::string& errorMsg) {
    if (check(type)) {
        return advance();
    }
    std::ostringstream oss;
    oss << "Config error at line " << current().line << ": " << errorMsg
        << " (got '" << current().value << "')";
    throw ConfigError(oss.str());
}

void ConfigParser::expectSemicolon() {
    consume(TOKEN_SEMICOLON, "Expected ';' at the end of directive");
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& configFilePath) {
    _tokens = ConfigTokenizer::tokenizeFile(configFilePath);
    _pos = 0;
    std::vector<ServerConfig> servers;

    while (!check(TOKEN_EOF)) {
        if (current().value == "server") {
            parseServer(servers);
        } else {
            std::ostringstream oss;
            oss << "Config error at line " << current().line
                << ": Unexpected token outside server block: '" << current().value << "'";
            throw ConfigError(oss.str());
        }
    }

    if (servers.empty()) {
        throw ConfigError("Configuration file contains no server blocks");
    }

    return servers;
}

std::vector<ServerConfig> ConfigParser::parseContent(const std::string& content) {
    _tokens = ConfigTokenizer::tokenize(content);
    _pos = 0;
    std::vector<ServerConfig> servers;

    while (!check(TOKEN_EOF)) {
        if (current().value == "server") {
            parseServer(servers);
        } else {
            std::ostringstream oss;
            oss << "Config error at line " << current().line
                << ": Unexpected token outside server block: '" << current().value << "'";
            throw ConfigError(oss.str());
        }
    }

    if (servers.empty()) {
        throw ConfigError("Configuration contains no server blocks");
    }

    return servers;
}

void ConfigParser::parseServer(std::vector<ServerConfig>& servers) {
    advance(); // consume "server"
    consume(TOKEN_BRACE_OPEN, "Expected '{' after 'server'");

    ServerConfig server;
    while (!check(TOKEN_BRACE_CLOSE) && !check(TOKEN_EOF)) {
        parseServerDirective(server);
    }

    consume(TOKEN_BRACE_CLOSE, "Expected '}' to close server block");
    servers.push_back(server);
}

void ConfigParser::parseServerDirective(ServerConfig& server) {
    std::string directive = current().value;

    if (directive == "listen") {
        parseListen(server);
    } else if (directive == "server_name") {
        parseServerName(server);
    } else if (directive == "error_page") {
        parseErrorPage(server);
    } else if (directive == "client_max_body_size") {
        parseClientMaxBodySize(server);
    } else if (directive == "root") {
        parseServerRoot(server);
    } else if (directive == "index") {
        parseServerIndex(server);
    } else if (directive == "location") {
        parseLocation(server);
    } else {
        std::ostringstream oss;
        oss << "Unknown server directive '" << directive << "' at line " << current().line;
        throw ConfigError(oss.str());
    }
}

void ConfigParser::parseListen(ServerConfig& server) {
    advance(); // consume "listen"
    Token tok = consume(TOKEN_WORD, "Expected host:port or port after 'listen'");
    std::string val = tok.value;

    size_t colon = val.find(':');
    if (colon != std::string::npos) {
        std::string host = val.substr(0, colon);
        std::string portStr = val.substr(colon + 1);
        int port = StringUtils::toInt(portStr, -1);
        if (port <= 0 || port > 65535) {
            std::ostringstream oss;
            oss << "Invalid port '" << portStr << "' at line " << tok.line;
            throw ConfigError(oss.str());
        }
        server.setHost(host);
        server.addPort(port);
    } else {
        int port = StringUtils::toInt(val, -1);
        if (port <= 0 || port > 65535) {
            std::ostringstream oss;
            oss << "Invalid port '" << val << "' at line " << tok.line;
            throw ConfigError(oss.str());
        }
        server.addPort(port);
    }

    expectSemicolon();
}

void ConfigParser::parseServerName(ServerConfig& server) {
    advance(); // consume "server_name"
    if (check(TOKEN_SEMICOLON)) {
        throw ConfigError("Expected at least one server name at line " + StringUtils::toString(current().line));
    }
    while (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        Token tok = consume(TOKEN_WORD, "Expected server name");
        server.addServerName(tok.value);
    }
    expectSemicolon();
}

void ConfigParser::parseErrorPage(ServerConfig& server) {
    advance(); // consume "error_page"
    std::vector<int> codes;
    while (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        if (peek().type == TOKEN_SEMICOLON && !codes.empty()) {
            // Last word is the path
            Token pathTok = consume(TOKEN_WORD, "Expected error page path");
            for (size_t i = 0; i < codes.size(); ++i) {
                server.addErrorPage(codes[i], pathTok.value);
            }
            break;
        } else {
            Token codeTok = consume(TOKEN_WORD, "Expected error code or path");
            int code = StringUtils::toInt(codeTok.value, -1);
            if (code < 100 || code > 599) {
                if (codes.empty()) {
                    std::ostringstream oss;
                    oss << "Invalid error code '" << codeTok.value << "' at line " << codeTok.line;
                    throw ConfigError(oss.str());
                } else {
                    // It's the path
                    for (size_t i = 0; i < codes.size(); ++i) {
                        server.addErrorPage(codes[i], codeTok.value);
                    }
                    break;
                }
            } else {
                codes.push_back(code);
            }
        }
    }
    expectSemicolon();
}

void ConfigParser::parseClientMaxBodySize(ServerConfig& server) {
    advance(); // consume "client_max_body_size"
    Token tok = consume(TOKEN_WORD, "Expected size after 'client_max_body_size'");
    size_t size = StringUtils::parseByteSize(tok.value);
    server.setClientMaxBodySize(size);
    expectSemicolon();
}

void ConfigParser::parseServerRoot(ServerConfig& server) {
    advance(); // consume "root"
    Token tok = consume(TOKEN_WORD, "Expected root path");
    server.setRoot(tok.value);
    expectSemicolon();
}

void ConfigParser::parseServerIndex(ServerConfig& server) {
    advance(); // consume "index"
    Token tok = consume(TOKEN_WORD, "Expected index filename");
    server.setIndex(tok.value);
    while (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        advance(); // consume extra index files if any
    }
    expectSemicolon();
}

void ConfigParser::parseLocation(ServerConfig& server) {
    advance(); // consume "location"
    Token pathTok = consume(TOKEN_WORD, "Expected location path");
    consume(TOKEN_BRACE_OPEN, "Expected '{' after location path");

    LocationConfig location;
    location.setPath(pathTok.value);

    while (!check(TOKEN_BRACE_CLOSE) && !check(TOKEN_EOF)) {
        parseLocationDirective(location);
    }

    consume(TOKEN_BRACE_CLOSE, "Expected '}' to close location block");
    server.addLocation(location);
}

void ConfigParser::parseLocationDirective(LocationConfig& location) {
    std::string directive = current().value;

    if (directive == "root") {
        parseLocationRoot(location);
    } else if (directive == "index") {
        parseLocationIndex(location);
    } else if (directive == "autoindex") {
        parseLocationAutoindex(location);
    } else if (directive == "methods" || directive == "allow_methods" || directive == "accepted_methods" || directive == "limit_except") {
        parseLocationMethods(location);
    } else if (directive == "return" || directive == "redirect") {
        parseLocationReturn(location);
    } else if (directive == "upload_dir" || directive == "upload_store" || directive == "upload_pass") {
        parseLocationUploadDir(location);
    } else if (directive == "cgi_ext" || directive == "cgi_pass" || directive == "cgi_bin") {
        parseLocationCgi(location);
    } else if (directive == "client_max_body_size") {
        parseLocationBodySize(location);
    } else {
        std::ostringstream oss;
        oss << "Unknown location directive '" << directive << "' at line " << current().line;
        throw ConfigError(oss.str());
    }
}

void ConfigParser::parseLocationRoot(LocationConfig& location) {
    advance(); // consume "root"
    Token tok = consume(TOKEN_WORD, "Expected root path");
    location.setRoot(tok.value);
    expectSemicolon();
}

void ConfigParser::parseLocationIndex(LocationConfig& location) {
    advance(); // consume "index"
    Token tok = consume(TOKEN_WORD, "Expected index filename");
    location.setIndex(tok.value);
    while (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        advance();
    }
    expectSemicolon();
}

void ConfigParser::parseLocationAutoindex(LocationConfig& location) {
    advance(); // consume "autoindex"
    Token tok = consume(TOKEN_WORD, "Expected 'on' or 'off' for autoindex");
    std::string val = StringUtils::toLower(tok.value);
    if (val == "on") {
        location.setAutoindex(true);
    } else if (val == "off") {
        location.setAutoindex(false);
    } else {
        std::ostringstream oss;
        oss << "Invalid autoindex value '" << tok.value << "' at line " << tok.line;
        throw ConfigError(oss.str());
    }
    expectSemicolon();
}

void ConfigParser::parseLocationMethods(LocationConfig& location) {
    advance(); // consume "methods"
    int methods = 0;
    while (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        Token tok = consume(TOKEN_WORD, "Expected HTTP method");
        std::string m = StringUtils::toUpper(tok.value);
        HttpMethod hm = stringToMethod(m);
        if (hm == METHOD_UNKNOWN) {
            std::ostringstream oss;
            oss << "Unsupported HTTP method '" << tok.value << "' at line " << tok.line;
            throw ConfigError(oss.str());
        }
        methods |= hm;
    }
    location.setAllowedMethods(methods);
    expectSemicolon();
}

void ConfigParser::parseLocationReturn(LocationConfig& location) {
    advance(); // consume "return"
    Token tok1 = consume(TOKEN_WORD, "Expected return code or URL");
    if (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
        // tok1 is status code, next token is URL
        int code = StringUtils::toInt(tok1.value, 302);
        Token tok2 = consume(TOKEN_WORD, "Expected return URL");
        location.setRedirect(code, tok2.value);
    } else {
        // Just URL with default 302
        location.setRedirect(302, tok1.value);
    }
    expectSemicolon();
}

void ConfigParser::parseLocationUploadDir(LocationConfig& location) {
    advance(); // consume "upload_dir"
    Token tok = consume(TOKEN_WORD, "Expected upload directory path");
    location.setUploadDir(tok.value);
    expectSemicolon();
}

void ConfigParser::parseLocationCgi(LocationConfig& location) {
    std::string dir = advance().value; // "cgi_ext", "cgi_pass", or "cgi_bin"
    if (dir == "cgi_ext") {
        Token extTok = consume(TOKEN_WORD, "Expected file extension for cgi_ext");
        std::string binPath = "";
        if (!check(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
            Token binTok = consume(TOKEN_WORD, "Expected interpreter path");
            binPath = binTok.value;
        }
        std::string ext = extTok.value;
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
        location.addCgiHandler(ext, binPath);
    } else if (dir == "cgi_pass") {
        Token extTok = consume(TOKEN_WORD, "Expected file extension for cgi_pass");
        Token binTok = consume(TOKEN_WORD, "Expected interpreter path for cgi_pass");
        std::string ext = extTok.value;
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
        location.addCgiHandler(ext, binTok.value);
    } else if (dir == "cgi_bin") {
        Token binTok = consume(TOKEN_WORD, "Expected interpreter path for cgi_bin");
        location.addCgiHandler(".cgi", binTok.value);
        location.addCgiHandler(".py", binTok.value);
        location.addCgiHandler(".sh", binTok.value);
        location.addCgiHandler(".php", binTok.value);
    }
    expectSemicolon();
}

void ConfigParser::parseLocationBodySize(LocationConfig& location) {
    advance(); // consume "client_max_body_size"
    Token tok = consume(TOKEN_WORD, "Expected size after 'client_max_body_size'");
    size_t size = StringUtils::parseByteSize(tok.value);
    location.setClientMaxBodySize(size);
    expectSemicolon();
}
