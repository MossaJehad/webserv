#include "DeleteHandler.hpp"
#include "ErrorResponse.hpp"
#include "FileSystem.hpp"

DeleteHandler::DeleteHandler() {}
DeleteHandler::~DeleteHandler() {}

HttpResponse DeleteHandler::handle(const RequestContext& ctx) {
    std::string fsPath = ctx.getResolvedFsPath();

    if (!FileSystem::exists(fsPath)) {
        return ErrorResponse::build(404, ctx.getServer());
    }

    if (FileSystem::isDir(fsPath)) {
        if (!FileSystem::deleteDirRecursive(fsPath)) {
            return ErrorResponse::build(403, ctx.getServer());
        }
    } else {
        if (!FileSystem::deleteFile(fsPath)) {
            return ErrorResponse::build(403, ctx.getServer());
        }
    }

    HttpResponse response(200);
    std::string body = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head><title>200 Deleted</title></head>\n"
                       "<body style=\"font-family: Arial, sans-serif; text-align: center; margin-top: 100px;\">\n"
                       "  <h1>200 File Deleted Successfully</h1>\n"
                       "  <p>The requested resource has been deleted.</p>\n"
                       "</body>\n"
                       "</html>\n";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}
