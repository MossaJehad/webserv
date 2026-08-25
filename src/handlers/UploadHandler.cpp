#include "UploadHandler.hpp"
#include "ErrorResponse.hpp"
#include "FileSystem.hpp"
#include "StringUtils.hpp"
#include "Time.hpp"

UploadHandler::UploadHandler() {}
UploadHandler::~UploadHandler() {}

static bool parseMultipart(const std::string& body, const std::string& boundary,
                           std::string& filename, std::string& fileContent) {
    std::string delimiter = "--" + boundary;
    size_t startPos = body.find(delimiter);
    if (startPos == std::string::npos) {
        return false;
    }

    startPos += delimiter.size();
    if (startPos + 2 < body.size() && body.substr(startPos, 2) == "\r\n") {
        startPos += 2;
    }

    size_t endPos = body.find(delimiter, startPos);
    if (endPos == std::string::npos) {
        return false;
    }

    std::string part = body.substr(startPos, endPos - startPos);

    // Find header boundary (\r\n\r\n) in part
    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }

    std::string headers = part.substr(0, headerEnd);
    fileContent = part.substr(headerEnd + 4);

    // Remove trailing \r\n from fileContent
    if (fileContent.size() >= 2 && fileContent.substr(fileContent.size() - 2) == "\r\n") {
        fileContent = fileContent.substr(0, fileContent.size() - 2);
    }

    // Parse filename from Content-Disposition: form-data; name="file"; filename="photo.jpg"
    size_t fnPos = headers.find("filename=\"");
    if (fnPos != std::string::npos) {
        fnPos += 10;
        size_t quoteEnd = headers.find('"', fnPos);
        if (quoteEnd != std::string::npos) {
            filename = headers.substr(fnPos, quoteEnd - fnPos);
        }
    }

    return !filename.empty();
}

HttpResponse UploadHandler::handle(const RequestContext& ctx) {
    const LocationConfig& loc = ctx.getLocation();
    std::string uploadDir = loc.getUploadDir();

    if (uploadDir.empty()) {
        uploadDir = loc.getRoot();
    }

    // Ensure upload directory exists or try to resolve it
    if (!FileSystem::exists(uploadDir)) {
        return ErrorResponse::build(500, ctx.getServer());
    }

    const HttpRequest& req = ctx.getRequest();
    std::string contentType = req.getHeaders().get("Content-Type");
    std::string filename;
    std::string fileContent;

    if (contentType.find("multipart/form-data") != std::string::npos) {
        size_t bPos = contentType.find("boundary=");
        if (bPos != std::string::npos) {
            std::string boundary = contentType.substr(bPos + 9);
            size_t semi = boundary.find(';');
            if (semi != std::string::npos) {
                boundary = boundary.substr(0, semi);
            }
            boundary = StringUtils::trim(boundary);
            if (!boundary.empty() && boundary[0] == '"' && boundary[boundary.size() - 1] == '"') {
                boundary = boundary.substr(1, boundary.size() - 2);
            }
            parseMultipart(req.getBody(), boundary, filename, fileContent);
        }
    }

    // Fallback if not multipart or multipart parse failed to extract filename
    if (filename.empty()) {
        std::string reqFile = FileSystem::getFilename(req.getPath());
        if (!reqFile.empty() && reqFile != "/" && reqFile != FileSystem::getFilename(loc.getPath())) {
            filename = reqFile;
        } else {
            filename = "upload_" + StringUtils::toString(Time::now()) + ".bin";
        }
        fileContent = req.getBody();
    }

    // Sanitize filename to prevent directory traversal
    filename = FileSystem::getFilename(filename);
    if (filename.empty()) {
        filename = "upload_" + StringUtils::toString(Time::now()) + ".bin";
    }

    std::string destPath = FileSystem::joinPaths(uploadDir, filename);

    if (!FileSystem::writeFile(destPath, fileContent)) {
        return ErrorResponse::build(500, ctx.getServer());
    }

    HttpResponse response(201);
    std::string locHeader = FileSystem::joinPaths(loc.getPath(), filename);
    response.getHeaders().set("Location", locHeader);

    std::string body = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head><title>201 Created</title></head>\n"
                       "<body style=\"font-family: Arial, sans-serif; text-align: center; margin-top: 100px;\">\n"
                       "  <h1>201 File Created Successfully</h1>\n"
                       "  <p>File saved to: " + filename + " (" + StringUtils::toString(fileContent.size()) + " bytes)</p>\n"
                       "  <a href=\"" + locHeader + "\">View File</a>\n"
                       "</body>\n"
                       "</html>\n";
    response.setBody(body);
    response.setContentType("text/html");

    return response;
}
