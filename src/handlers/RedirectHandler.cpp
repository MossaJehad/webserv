#include "RedirectHandler.hpp"

RedirectHandler::RedirectHandler() {}
RedirectHandler::~RedirectHandler() {}

HttpResponse RedirectHandler::handle(const RequestContext& ctx) {
    int code = ctx.getLocation().getRedirectCode();
    if (code <= 0) {
        code = 302;
    }
    return HttpResponse::redirect(ctx.getLocation().getRedirectUrl(), code);
}
