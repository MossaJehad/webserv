#ifndef IREQUESTHANDLER_HPP
#define IREQUESTHANDLER_HPP

#include "RequestContext.hpp"
#include "HttpResponse.hpp"

class IRequestHandler {
public:
    virtual ~IRequestHandler() {}
    virtual HttpResponse handle(const RequestContext& ctx) = 0;
};

#endif
