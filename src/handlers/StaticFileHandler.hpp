#ifndef STATICFILEHANDLER_HPP
#define STATICFILEHANDLER_HPP

#include "IRequestHandler.hpp"

class StaticFileHandler : public IRequestHandler {
public:
    StaticFileHandler();
    virtual ~StaticFileHandler();

    virtual HttpResponse handle(const RequestContext& ctx);
};

#endif
