#include "UploadHandler.hpp"
#include "../http/HttpResponse.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>

UploadHandler::UploadHandler() {}
UploadHandler::~UploadHandler() {}

void UploadHandler::handle(const ServerConfig& srv,
                           const LocationConfig& loc,
                           const HttpRequest& req,
                           HttpResponse& resp)
{
    (void)srv;
    if (loc.uploadPath.empty()) {
        resp.setStatus(403);
        resp.setBody(HttpResponse::buildError(403), "text/html");
        return;
    }

    std::string contentType = req.getHeader("content-type");
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos != std::string::npos) {
        std::string boundary = contentType.substr(boundaryPos + 9);
        // Remove any trailing params
        size_t semi = boundary.find(';');
        if (semi != std::string::npos) boundary = boundary.substr(0, semi);
        handleMultipart(boundary, loc, req, resp);
    } else {
        handleRaw(loc, req, resp);
    }
}

bool UploadHandler::handleMultipart(const std::string& boundary,
                                    const LocationConfig& loc,
                                    const HttpRequest& req,
                                    HttpResponse& resp)
{
    const std::string& body = req.getBody();
    std::string delim = "--" + boundary;
    std::string closingDelim = delim + "--";

    size_t pos = 0;
    int fileCount = 0;

    while (pos < body.size()) {
        // Find delimiter
        size_t delimPos = body.find(delim, pos);
        if (delimPos == std::string::npos) break;

        pos = delimPos + delim.size();
        if (pos + 2 > body.size()) break;

        // Check for closing delimiter
        if (body.substr(pos, 2) == "--") break;

        // Skip CRLF after delimiter
        if (body.substr(pos, 2) == "\r\n") pos += 2;
        else break;

        // Parse part headers
        size_t headerEnd = body.find("\r\n\r\n", pos);
        if (headerEnd == std::string::npos) break;

        std::string partHeaders = body.substr(pos, headerEnd - pos);
        pos = headerEnd + 4;

        // Find next delimiter to get part body
        size_t nextDelim = body.find("\r\n" + delim, pos);
        if (nextDelim == std::string::npos) break;

        std::string partBody = body.substr(pos, nextDelim - pos);
        pos = nextDelim + 2;

        // Extract filename from Content-Disposition
        size_t cdPos = partHeaders.find("Content-Disposition:");
        if (cdPos == std::string::npos) continue;

        size_t cdEnd = partHeaders.find("\r\n", cdPos);
        std::string cd = partHeaders.substr(cdPos, cdEnd == std::string::npos ? std::string::npos : cdEnd - cdPos);

        std::string filename = extractFilename(cd);
        if (filename.empty()) continue;

        // Sanitize filename
        for (size_t i = 0; i < filename.size(); ++i)
            if (filename[i] == '/' || filename[i] == '\\') filename[i] = '_';

        if (!ensureDirectory(loc.uploadPath)) {
            resp.setStatus(500);
            resp.setBody(HttpResponse::buildError(500), "text/html");
            return false;
        }

        std::string dest = loc.uploadPath;
        if (dest[dest.size()-1] != '/') dest += '/';
        dest += filename;

        if (!writeFile(dest, partBody)) {
            resp.setStatus(500);
            resp.setBody(HttpResponse::buildError(500), "text/html");
            return false;
        }
        ++fileCount;
    }

    if (fileCount == 0) {
        resp.setStatus(400);
        resp.setBody(HttpResponse::buildError(400), "text/html");
        return false;
    }

    std::ostringstream body_resp;
    body_resp << "<!DOCTYPE html><html><body><h2>Upload successful</h2>"
              << "<p>" << fileCount << " file(s) uploaded.</p>"
              << "<a href=\"/\">Back</a></body></html>";
    resp.setStatus(201);
    resp.setBody(body_resp.str(), "text/html");
    return true;
}

bool UploadHandler::handleRaw(const LocationConfig& loc,
                              const HttpRequest& req,
                              HttpResponse& resp)
{
    // Use a timestamp-based filename
    std::ostringstream name;
    name << "upload_" << (long)time(NULL);

    if (!ensureDirectory(loc.uploadPath)) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return false;
    }

    std::string dest = loc.uploadPath;
    if (dest[dest.size()-1] != '/') dest += '/';
    dest += name.str();

    if (!writeFile(dest, req.getBody())) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return false;
    }

    resp.setStatus(201);
    resp.setBody("<!DOCTYPE html><html><body><h2>Upload successful</h2></body></html>",
                 "text/html");
    return true;
}

std::string UploadHandler::extractFilename(const std::string& cd) const
{
    size_t pos = cd.find("filename=\"");
    if (pos == std::string::npos) return "";
    pos += 10;
    size_t end = cd.find('"', pos);
    if (end == std::string::npos) return "";
    return cd.substr(pos, end - pos);
}

bool UploadHandler::writeFile(const std::string& path, const std::string& data) const
{
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return false;
    f.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    return f.good();
}

bool UploadHandler::ensureDirectory(const std::string& path) const
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
    return mkdir(path.c_str(), 0755) == 0;
}
