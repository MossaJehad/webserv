#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "RequestContext.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "HttpRequest.hpp"
#include <vector>
#include <string>

class Router {
public:
    static const ServerConfig& matchServer(const std::vector<ServerConfig>& servers,
                                          const std::string& hostHeader,
                                          int serverPort);

    static const LocationConfig& matchLocation(const ServerConfig& server,
                                              const std::string& uriPath);

    static std::string resolveFsPath(const LocationConfig& location,
                                    const std::string& uriPath);

    static RequestContext route(const std::vector<ServerConfig>& servers,
                                const HttpRequest& req,
                                int serverPort);
};

#endif
