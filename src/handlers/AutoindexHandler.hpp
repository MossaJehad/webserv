#ifndef AUTOINDEXHANDLER_HPP
#define AUTOINDEXHANDLER_HPP

#include "IRequestHandler.hpp"
#include <string>

class AutoindexHandler : public IRequestHandler {
public:
    AutoindexHandler();
    virtual ~AutoindexHandler();

    virtual HttpResponse handle(const RequestContext& ctx);
    static HttpResponse generateListing(const std::string& dirPath, const std::string& uriPath);
};

#endif
