#include "Router.hpp"
#include "Exceptions.hpp"
#include "FileSystem.hpp"
#include "StringUtils.hpp"
#include "HttpStatus.hpp"

namespace {

// Comma-separated list of the methods this route permits, for the Allow header
// that RFC 7231 6.5.5 requires on a 405 response.
std::string allowedMethodList(const LocationConfig& location) {
    const HttpMethod known[] = { METHOD_GET, METHOD_HEAD, METHOD_POST,
                                 METHOD_DELETE, METHOD_PUT };
    std::string list;
    for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); ++i) {
        if (location.isMethodAllowed(known[i])) {
            if (!list.empty()) {
                list += ", ";
            }
            list += methodToString(known[i]);
        }
    }
    return list;
}

} // namespace

const ServerConfig& Router::matchServer(const std::vector<ServerConfig>& servers,
                                        const std::string& hostHeader,
                                        int serverPort) {
    std::string cleanHost = hostHeader;
    size_t colon = cleanHost.find(':');
    if (colon != std::string::npos) {
        cleanHost = cleanHost.substr(0, colon);
    }
    cleanHost = StringUtils::trim(cleanHost);

    const ServerConfig* firstMatchingPort = NULL;

    for (size_t i = 0; i < servers.size(); ++i) {
        if (servers[i].listensOnPort(serverPort)) {
            if (!firstMatchingPort) {
                firstMatchingPort = &servers[i];
            }
            if (!cleanHost.empty() && servers[i].matchesServerName(cleanHost)) {
                return servers[i];
            }
        }
    }

    if (firstMatchingPort) {
        return *firstMatchingPort;
    }

    // Fallback to first server in config
    return servers[0];
}

const LocationConfig& Router::matchLocation(const ServerConfig& server,
                                            const std::string& uriPath) {
    const std::vector<LocationConfig>& locations = server.getLocations();
    const LocationConfig* bestMatch = NULL;
    size_t longestMatchLen = 0;

    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& locPath = locations[i].getPath();
        if (uriPath == locPath) {
            return locations[i];
        }
        if (StringUtils::startsWith(uriPath, locPath)) {
            // Ensure prefix match boundaries (e.g. /upload should match /upload/file but not /uploads)
            if (locPath[locPath.size() - 1] == '/' || uriPath.size() == locPath.size() || uriPath[locPath.size()] == '/') {
                if (locPath.size() > longestMatchLen) {
                    longestMatchLen = locPath.size();
                    bestMatch = &locations[i];
                }
            }
        }
    }

    if (bestMatch) {
        return *bestMatch;
    }

    // Fallback to first location
    return locations[0];
}

std::string Router::resolveFsPath(const LocationConfig& location,
                                  const std::string& uriPath) {
    std::string decodedPath = StringUtils::urlDecode(uriPath);
    std::string normalizedPath = FileSystem::normalizePath(decodedPath);

    std::string locPath = location.getPath();
    std::string root = location.getRoot();

    std::string relative;
    if (StringUtils::startsWith(normalizedPath, locPath)) {
        relative = normalizedPath.substr(locPath.size());
    } else {
        relative = normalizedPath;
    }

    if (!relative.empty() && relative[0] == '/') {
        relative = relative.substr(1);
    }

    return FileSystem::joinPaths(root, relative);
}

RequestContext Router::route(const std::vector<ServerConfig>& servers,
                            const HttpRequest& req,
                            int serverPort) {
    RequestContext ctx;
    ctx.setRequest(req);

    const ServerConfig& server = matchServer(servers, req.getHost(), serverPort);
    ctx.setServer(&server);

    const LocationConfig& location = matchLocation(server, req.getPath());
    ctx.setLocation(location);

    // RFC 7231 6.6.2 vs 6.5.5: a method the server does not implement at all is
    // 501, while 405 means the method is understood but not permitted on this
    // route (and then a valid response must advertise what is allowed).
    if (req.getMethod() == METHOD_UNKNOWN) {
        throw HttpError(STATUS_NOT_IMPLEMENTED, "Not Implemented");
    }

    if (!location.isMethodAllowed(req.getMethod())) {
        throw HttpError(STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed",
                        allowedMethodList(location));
    }

    // Client body size check
    size_t limit = location.hasBodySizeLimit() ? location.getClientMaxBodySize() : server.getClientMaxBodySize();
    if (limit > 0 && req.getBody().size() > limit) {
        throw HttpError(STATUS_PAYLOAD_TOO_LARGE, "Payload Too Large");
    }

    // Resolve filesystem path
    std::string fsPath = resolveFsPath(location, req.getPath());
    ctx.setResolvedFsPath(fsPath);

    // Check if CGI should be triggered
    // Check direct file extension match or PATH_INFO style
    const std::map<std::string, std::string>& cgiPass = location.getCgiPass();
    if (!cgiPass.empty()) {
        std::string reqPath = req.getPath();
        for (std::map<std::string, std::string>::const_iterator it = cgiPass.begin(); it != cgiPass.end(); ++it) {
            const std::string& ext = it->first;
            size_t extPos = reqPath.find(ext);
            if (extPos != std::string::npos) {
                size_t afterExt = extPos + ext.size();
                if (afterExt == reqPath.size() || reqPath[afterExt] == '/' || reqPath[afterExt] == '?') {
                    std::string scriptUri = reqPath.substr(0, afterExt);
                    std::string pathInfo = (afterExt < reqPath.size()) ? reqPath.substr(afterExt) : "";

                    ctx.setIsCgi(true);
                    ctx.setScriptPath(resolveFsPath(location, scriptUri));
                    ctx.setPathInfo(pathInfo);
                    ctx.setCgiBin(it->second);
                    break;
                }
            }
        }
    }

    return ctx;
}
