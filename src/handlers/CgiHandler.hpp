#pragma once
#include <string>
#include <vector>
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

class CgiHandler {
public:
    CgiHandler();
    ~CgiHandler();

    // Synchronous CGI execution (used after poll says fd is ready)
    void handle(const ServerConfig& srv,
                const LocationConfig& loc,
                const HttpRequest& req,
                HttpResponse& resp,
                const std::string& scriptPath);

private:
    std::vector<std::string> buildEnv(const ServerConfig& srv,
                                      const LocationConfig& loc,
                                      const HttpRequest& req,
                                      const std::string& scriptPath) const;

    std::string findInterpreter(const std::string& ext,
                                const LocationConfig& loc) const;

    void parseCgiOutput(const std::string& raw, HttpResponse& resp) const;

    char** vecToCharPP(const std::vector<std::string>& v) const;
    void freeCharPP(char** pp) const;
};
