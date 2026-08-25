#include "CgiEnvironment.hpp"
#include "StringUtils.hpp"
#include "FileSystem.hpp"
#include <cstring>
#include <cstdlib>

CgiEnvironment::CgiEnvironment() {}
CgiEnvironment::~CgiEnvironment() {}

void CgiEnvironment::build(const RequestContext& ctx) {
    _envMap.clear();

    const HttpRequest& req = ctx.getRequest();
    const ServerConfig* server = ctx.getServer();

    _envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    _envMap["SERVER_PROTOCOL"]   = "HTTP/1.1";
    _envMap["SERVER_SOFTWARE"]   = "webserv/1.0";
    _envMap["REDIRECT_STATUS"]   = "200"; // For PHP-CGI

    _envMap["REQUEST_METHOD"]    = req.getRawMethod().empty() ? "GET" : req.getRawMethod();
    _envMap["QUERY_STRING"]      = req.getQuery();
    _envMap["SCRIPT_NAME"]       = req.getPath();
    _envMap["SCRIPT_FILENAME"]   = ctx.getScriptPath().empty() ? ctx.getResolvedFsPath() : ctx.getScriptPath();

    if (!ctx.getPathInfo().empty()) {
        _envMap["PATH_INFO"]       = ctx.getPathInfo();
        _envMap["PATH_TRANSLATED"] = FileSystem::joinPaths(ctx.getLocation().getRoot(), ctx.getPathInfo());
    }

    if (server) {
        _envMap["SERVER_NAME"] = server->getServerNames().empty() ? "localhost" : server->getServerNames()[0];
        _envMap["SERVER_PORT"] = StringUtils::toString(req.getPort() > 0 ? req.getPort() : (server->getPorts().empty() ? 8080 : server->getPorts()[0]));
    } else {
        _envMap["SERVER_NAME"] = "localhost";
        _envMap["SERVER_PORT"] = "8080";
    }

    _envMap["REMOTE_ADDR"] = req.getClientIp().empty() ? "127.0.0.1" : req.getClientIp();
    _envMap["REMOTE_HOST"] = _envMap["REMOTE_ADDR"];
    _envMap["REMOTE_PORT"] = StringUtils::toString(req.getClientPort());

    if (req.getHeaders().has("Content-Type")) {
        _envMap["CONTENT_TYPE"] = req.getHeaders().get("Content-Type");
    }
    if (req.getHeaders().has("Content-Length")) {
        _envMap["CONTENT_LENGTH"] = req.getHeaders().get("Content-Length");
    } else if (!req.getBody().empty()) {
        _envMap["CONTENT_LENGTH"] = StringUtils::toString(req.getBody().size());
    }

    // Add HTTP headers as HTTP_*
    const std::vector<std::pair<std::string, std::string> >& headerList = req.getHeaders().getList();
    for (size_t i = 0; i < headerList.size(); ++i) {
        std::string name = headerList[i].first;
        std::string varName = "HTTP_";
        for (size_t j = 0; j < name.size(); ++j) {
            if (name[j] == '-') {
                varName += '_';
            } else {
                varName += static_cast<char>(std::toupper(static_cast<unsigned char>(name[j])));
            }
        }
        _envMap[varName] = headerList[i].second;
    }
}

char** CgiEnvironment::createEnvp() const {
    char** envp = new char*[_envMap.size() + 1];
    size_t idx = 0;

    for (std::map<std::string, std::string>::const_iterator it = _envMap.begin(); it != _envMap.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        envp[idx] = new char[entry.size() + 1];
        std::strcpy(envp[idx], entry.c_str());
        idx++;
    }
    envp[idx] = NULL;
    return envp;
}

void CgiEnvironment::freeEnvp(char** envp) {
    if (!envp) return;
    for (size_t i = 0; envp[i] != NULL; ++i) {
        delete[] envp[i];
    }
    delete[] envp;
}

const std::map<std::string, std::string>& CgiEnvironment::getMap() const {
    return _envMap;
}
