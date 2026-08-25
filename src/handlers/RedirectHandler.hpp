#ifndef REDIRECTHANDLER_HPP
#define REDIRECTHANDLER_HPP

#include "IRequestHandler.hpp"

class RedirectHandler : public IRequestHandler {
public:
    RedirectHandler();
    virtual ~RedirectHandler();

    virtual HttpResponse handle(const RequestContext& ctx);
};

#endif
