#ifndef HANDLERFACTORY_HPP
#define HANDLERFACTORY_HPP

#include "IRequestHandler.hpp"
#include "RequestContext.hpp"

class HandlerFactory {
public:
    static IRequestHandler* createHandler(const RequestContext& ctx);
};

#endif
