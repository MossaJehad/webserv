#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "IRequestHandler.hpp"

class UploadHandler : public IRequestHandler {
public:
    UploadHandler();
    virtual ~UploadHandler();

    virtual HttpResponse handle(const RequestContext& ctx);
};

#endif
