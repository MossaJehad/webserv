#ifndef DELETEHANDLER_HPP
#define DELETEHANDLER_HPP

#include "IRequestHandler.hpp"

class DeleteHandler : public IRequestHandler {
public:
    DeleteHandler();
    virtual ~DeleteHandler();

    virtual HttpResponse handle(const RequestContext& ctx);
};

#endif
