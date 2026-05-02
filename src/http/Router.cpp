#include "Router.hpp"
#include <sstream>

Router::Router() {}
Router::~Router() {}

const LocationConfig* Router::route(const ServerConfig& server,
                                    const HttpRequest& request,
                                    HttpResponse& response)
{
    const LocationConfig* loc = server.matchLocation(request.getPath());
    if (!loc) {
        std::string errBody = HttpResponse::buildError(404);
        std::string errPage = server.getErrorPage(404);
        if (!errPage.empty()) {
            // errPage is a path; StaticHandler will load it
        }
        response.setStatus(404);
        response.setBody(HttpResponse::buildError(404, errPage), "text/html");
        return NULL;
    }

    // Handle redirect
    if (loc->redirectCode != 0) {
        response.setStatus(loc->redirectCode);
        response.setHeader("Location", loc->redirectUrl);
        response.setBody("", "text/html");
        return NULL;
    }

    return loc;
}

bool Router::checkMethod(const LocationConfig& loc,
                         const std::string& method,
                         HttpResponse& response)
{
    if (!loc.isMethodAllowed(method)) {
        // Build Allow header
        std::string allow;
        for (size_t i = 0; i < loc.allowedMethods.size(); ++i) {
            if (i > 0) allow += ", ";
            allow += loc.allowedMethods[i];
        }
        response.setStatus(405);
        response.setHeader("Allow", allow);
        response.setBody(HttpResponse::buildError(405), "text/html");
        return false;
    }
    return true;
}

bool Router::checkBodySize(const ServerConfig& srv,
                           const LocationConfig& loc,
                           const HttpRequest& req,
                           HttpResponse& response)
{
    long limit = loc.clientMaxBodySize > 0 ? loc.clientMaxBodySize : srv.clientMaxBodySize;
    if (limit > 0 && (long)req.getBody().size() > limit) {
        response.setStatus(413);
        response.setBody(HttpResponse::buildError(413), "text/html");
        return false;
    }
    return true;
}
