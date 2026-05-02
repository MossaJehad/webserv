#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

ConfigParser::ConfigParser() : _pos(0) {}
ConfigParser::~ConfigParser() {}

const std::vector<ServerConfig>& ConfigParser::getServers() const { return _servers; }
const std::string& ConfigParser::getError() const { return _error; }

bool ConfigParser::parse(const std::string& filename)
{
    if (!tokenize(filename))
        return false;
    _pos = 0;
    while (_pos < _tokens.size()) {
        std::string tok = nextToken();
        if (tok == "server") {
            if (!parseServer()) return false;
        } else {
            _error = "Unexpected token at top level: " + tok;
            return false;
        }
    }
    if (_servers.empty()) {
        _error = "No server blocks found in config.";
        return false;
    }
    return true;
}

bool ConfigParser::tokenize(const std::string& filename)
{
    std::ifstream f(filename.c_str());
    if (!f.is_open()) {
        _error = "Cannot open config file: " + filename;
        return false;
    }
    std::string line;
    while (std::getline(f, line)) {
        // Strip comments
        size_t cpos = line.find('#');
        if (cpos != std::string::npos)
            line = line.substr(0, cpos);

        std::istringstream iss(line);
        std::string word;
        while (iss >> word) {
            // Split on { and }
            for (size_t i = 0; i < word.size(); ) {
                if (word[i] == '{' || word[i] == '}' || word[i] == ';') {
                    if (i > 0)
                        _tokens.push_back(word.substr(0, i));
                    _tokens.push_back(word.substr(i, 1));
                    word = word.substr(i + 1);
                    i = 0;
                } else {
                    ++i;
                }
            }
            if (!word.empty())
                _tokens.push_back(word);
        }
    }
    return true;
}

std::string ConfigParser::nextToken()
{
    if (_pos >= _tokens.size()) return "";
    return _tokens[_pos++];
}

std::string ConfigParser::peekToken() const
{
    if (_pos >= _tokens.size()) return "";
    return _tokens[_pos];
}

bool ConfigParser::expect(const std::string& tok)
{
    std::string got = nextToken();
    if (got != tok) {
        _error = "Expected '" + tok + "' but got '" + got + "'";
        return false;
    }
    return true;
}

bool ConfigParser::parseServer()
{
    if (!expect("{")) return false;

    ServerConfig srv;

    while (peekToken() != "}" && _pos < _tokens.size()) {
        std::string key = nextToken();

        if (key == "listen") {
            std::string val = nextToken();
            // Could be "host:port" or just "port"
            size_t colon = val.find(':');
            if (colon != std::string::npos) {
                srv.host = val.substr(0, colon);
                srv.port = std::atoi(val.substr(colon + 1).c_str());
            } else {
                srv.port = std::atoi(val.c_str());
            }
            if (!expect(";")) return false;
        } else if (key == "server_name") {
            while (peekToken() != ";") {
                srv.serverNames.push_back(nextToken());
            }
            if (!expect(";")) return false;
        } else if (key == "client_max_body_size") {
            srv.clientMaxBodySize = parseSize(nextToken());
            if (!expect(";")) return false;
        } else if (key == "error_page") {
            if (!parseErrorPage(srv)) return false;
        } else if (key == "location") {
            if (!parseLocation(srv)) return false;
        } else if (key == "root") {
            // Default root for server block
            LocationConfig defaultLoc;
            defaultLoc.root = nextToken();
            if (!expect(";")) return false;
            // Apply to existing locations that have no root
            for (size_t i = 0; i < srv.locations.size(); ++i)
                if (srv.locations[i].root.empty())
                    srv.locations[i].root = defaultLoc.root;
        } else {
            // skip unknown directive
            while (peekToken() != ";" && peekToken() != "}" && _pos < _tokens.size())
                nextToken();
            if (peekToken() == ";") nextToken();
        }
    }
    if (!expect("}")) return false;

    // Ensure there's at least a "/" location
    bool hasRoot = false;
    for (size_t i = 0; i < srv.locations.size(); ++i) {
        if (srv.locations[i].path == "/") { hasRoot = true; break; }
    }
    if (!hasRoot) {
        LocationConfig def;
        def.path = "/";
        def.root = "./www/html";
        def.index = "index.html";
        srv.locations.push_back(def);
    }

    _servers.push_back(srv);
    return true;
}

bool ConfigParser::parseLocation(ServerConfig& srv)
{
    std::string path = nextToken();
    if (!expect("{")) return false;

    LocationConfig loc;
    loc.path = path;
    loc.clientMaxBodySize = srv.clientMaxBodySize;

    while (peekToken() != "}" && _pos < _tokens.size()) {
        std::string key = nextToken();

        if (key == "root") {
            loc.root = nextToken();
            if (!expect(";")) return false;
        } else if (key == "index") {
            loc.index = nextToken();
            if (!expect(";")) return false;
        } else if (key == "autoindex") {
            loc.autoindex = (nextToken() == "on");
            if (!expect(";")) return false;
        } else if (key == "allow_methods" || key == "limit_except") {
            loc.allowedMethods.clear();
            while (peekToken() != ";")
                loc.allowedMethods.push_back(nextToken());
            if (!expect(";")) return false;
        } else if (key == "return") {
            loc.redirectCode = std::atoi(nextToken().c_str());
            loc.redirectUrl  = nextToken();
            if (!expect(";")) return false;
        } else if (key == "upload_path") {
            loc.uploadPath = nextToken();
            if (!expect(";")) return false;
        } else if (key == "cgi_ext") {
            while (peekToken() != ";")
                loc.cgiExtensions.push_back(nextToken());
            if (!expect(";")) return false;
        } else if (key == "cgi_pass") {
            loc.cgiPass = nextToken();
            if (!expect(";")) return false;
        } else if (key == "client_max_body_size") {
            loc.clientMaxBodySize = parseSize(nextToken());
            if (!expect(";")) return false;
        } else {
            while (peekToken() != ";" && peekToken() != "}" && _pos < _tokens.size())
                nextToken();
            if (peekToken() == ";") nextToken();
        }
    }
    if (!expect("}")) return false;
    srv.locations.push_back(loc);
    return true;
}

bool ConfigParser::parseErrorPage(ServerConfig& srv)
{
    std::vector<int> codes;
    while (true) {
        std::string tok = peekToken();
        if (tok == ";") break;
        // if the token starts with a digit, it's a code
        if (!tok.empty() && tok[0] >= '0' && tok[0] <= '9') {
            codes.push_back(std::atoi(nextToken().c_str()));
        } else {
            // it's the path
            std::string pagePath = nextToken();
            for (size_t i = 0; i < codes.size(); ++i)
                srv.errorPages[codes[i]] = pagePath;
            break;
        }
    }
    if (!expect(";")) return false;
    return true;
}

long ConfigParser::parseSize(const std::string& val)
{
    if (val.empty()) return 1048576;
    char unit = val[val.size() - 1];
    long num = std::atol(val.c_str());
    if (unit == 'k' || unit == 'K') return num * 1024;
    if (unit == 'm' || unit == 'M') return num * 1024 * 1024;
    if (unit == 'g' || unit == 'G') return num * 1024 * 1024 * 1024;
    return num;
}
