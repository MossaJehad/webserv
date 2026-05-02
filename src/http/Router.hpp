#pragma once
#include <string>
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Router {
public:
    Router();
    ~Router();

    // Returns the matched location; NULL if none found
    const LocationConfig* route(const ServerConfig& server,
                                const HttpRequest& request,
                                HttpResponse& response);

    // Check if method is allowed; writes 405 to response if not
    bool checkMethod(const LocationConfig& loc,
                     const std::string& method,
                     HttpResponse& response);

    // Check body size
    bool checkBodySize(const ServerConfig& srv,
                       const LocationConfig& loc,
                       const HttpRequest& req,
                       HttpResponse& response);
};
