#include "HandlerFactory.hpp"
#include "RedirectHandler.hpp"
#include "UploadHandler.hpp"
#include "DeleteHandler.hpp"
#include "StaticFileHandler.hpp"

IRequestHandler* HandlerFactory::createHandler(const RequestContext& ctx) {
    if (ctx.getLocation().hasRedirect()) {
        return new RedirectHandler();
    }

    HttpMethod method = ctx.getRequest().getMethod();

    if (method == METHOD_POST) {
        return new UploadHandler();
    }

    if (method == METHOD_DELETE) {
        return new DeleteHandler();
    }

    // Default to StaticFileHandler for GET, HEAD, etc.
    return new StaticFileHandler();
}
