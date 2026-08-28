#include "StaticFileHandler.hpp"
#include "AutoindexHandler.hpp"
#include "ErrorResponse.hpp"
#include "FileSystem.hpp"
#include "MimeTypes.hpp"
#include "Time.hpp"
#include "StringUtils.hpp"

StaticFileHandler::StaticFileHandler() {}
StaticFileHandler::~StaticFileHandler() {}

HttpResponse StaticFileHandler::handle(const RequestContext& ctx) {
    std::string fsPath = ctx.getResolvedFsPath();

    if (!FileSystem::exists(fsPath)) {
        return ErrorResponse::build(404, ctx.getServer());
    }

    if (FileSystem::isDir(fsPath)) {
        // Enforce trailing slash on directories for proper relative link resolution
        std::string reqPath = ctx.getRequest().getPath();
        if (reqPath.empty() || reqPath[reqPath.size() - 1] != '/') {
            std::string redirectUri = reqPath + "/";
            if (!ctx.getRequest().getQuery().empty()) {
                redirectUri += "?" + ctx.getRequest().getQuery();
            }
            return HttpResponse::redirect(redirectUri, 301);
        }

        // Look for index file
        std::string indexName = ctx.getLocation().getIndex();
        if (indexName.empty()) {
            indexName = "index.html";
        }
        std::string indexPath = FileSystem::joinPaths(fsPath, indexName);

        if (FileSystem::exists(indexPath) && !FileSystem::isDir(indexPath)) {
            fsPath = indexPath;
        } else {
            if (ctx.getLocation().getAutoindex()) {
                return AutoindexHandler::generateListing(fsPath, reqPath);
            } else {
                return ErrorResponse::build(403, ctx.getServer());
            }
        }
    }

    if (!FileSystem::isReadable(fsPath)) {
        return ErrorResponse::build(403, ctx.getServer());
    }

    std::string fileContent;
    if (!FileSystem::readFile(fsPath, fileContent)) {
        return ErrorResponse::build(500, ctx.getServer());
    }

    HttpResponse response(200);
    response.setBody(fileContent);
    response.setContentType(MimeTypes::getType(fsPath));

    std::time_t mtime = FileSystem::getLastModifiedTime(fsPath);
    if (mtime > 0) {
        response.getHeaders().set("Last-Modified", Time::formatHttpDate(mtime));
    }

    return response;
}
