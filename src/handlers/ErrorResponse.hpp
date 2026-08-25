#ifndef ERRORRESPONSE_HPP
#define ERRORRESPONSE_HPP

#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

class ErrorResponse {
public:
    static HttpResponse build(int statusCode, const ServerConfig* server = NULL);
    static std::string getDefaultErrorHtml(int statusCode);
};

#endif
