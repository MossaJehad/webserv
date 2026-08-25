#include "ErrorResponse.hpp"
#include "HttpStatus.hpp"
#include "FileSystem.hpp"
#include "StringUtils.hpp"

std::string ErrorResponse::getDefaultErrorHtml(int statusCode) {
    std::string reason = HttpStatus::getReasonPhrase(statusCode);
    std::string codeStr = StringUtils::toString(statusCode);

    return "<!DOCTYPE html>\n"
           "<html>\n"
           "<head><title>" + codeStr + " " + reason + "</title></head>\n"
           "<body style=\"font-family: Arial, sans-serif; text-align: center; margin-top: 100px; background-color: #f8f9fa; color: #333;\">\n"
           "  <h1 style=\"font-size: 48px; margin-bottom: 10px;\">" + codeStr + "</h1>\n"
           "  <h2 style=\"font-size: 24px; color: #6c757d; font-weight: normal;\">" + reason + "</h2>\n"
           "  <hr style=\"width: 300px; margin: 30px auto; border: 0; border-top: 1px solid #dee2e6;\">\n"
           "  <p style=\"font-size: 14px; color: #adb5bd;\">webserv / 1.0</p>\n"
           "</body>\n"
           "</html>\n";
}

HttpResponse ErrorResponse::build(int statusCode, const ServerConfig* server) {
    HttpResponse response(statusCode);
    std::string htmlBody;

    if (server) {
        std::string customPagePath = server->getErrorPage(statusCode);
        if (!customPagePath.empty()) {
            std::string fullPath = FileSystem::joinPaths(server->getRoot(), customPagePath);
            if (!FileSystem::readFile(fullPath, htmlBody)) {
                // If not found in server root, check direct custom path
                FileSystem::readFile(customPagePath, htmlBody);
            }
        }
    }

    if (htmlBody.empty()) {
        htmlBody = getDefaultErrorHtml(statusCode);
    }

    response.setBody(htmlBody);
    response.setContentType("text/html");
    if (statusCode >= 400 && statusCode != 404 && statusCode != 405) {
        // If serious error or 400 Bad Request, close connection
        if (statusCode == 400 || statusCode == 408 || statusCode == 413 || statusCode == 500 || statusCode == 505) {
            response.setKeepAlive(false);
        }
    }

    return response;
}
