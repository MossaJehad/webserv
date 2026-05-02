#pragma once
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

class StaticHandler {
public:
    StaticHandler();
    ~StaticHandler();

    void handle(const ServerConfig& srv,
                const LocationConfig& loc,
                const HttpRequest& req,
                HttpResponse& resp);

private:
    void handleGet(const ServerConfig& srv,
                   const LocationConfig& loc,
                   const HttpRequest& req,
                   HttpResponse& resp);

    void handleDelete(const LocationConfig& loc,
                      const HttpRequest& req,
                      HttpResponse& resp);

    void serveFile(const std::string& fullPath, HttpResponse& resp);
    void serveAutoindex(const std::string& fullPath,
                        const std::string& uriPath,
                        HttpResponse& resp);

    std::string resolvePath(const LocationConfig& loc,
                            const std::string& uriPath) const;
    bool isDirectory(const std::string& path) const;
    bool fileExists(const std::string& path) const;
    std::string readFile(const std::string& path) const;
};
