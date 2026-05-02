#include "HttpResponse.hpp"
#include <sstream>
#include <map>

HttpResponse::HttpResponse() : _statusCode(200), _statusMsg("OK"), _ready(false) {}
HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code, const std::string& msg)
{
    _statusCode = code;
    _statusMsg  = msg.empty() ? statusMessage(code) : msg;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    _headers[name] = value;
}

void HttpResponse::setBody(const std::string& body, const std::string& contentType)
{
    _body = body;
    std::ostringstream ss;
    ss << body.size();
    _headers["Content-Length"] = ss.str();
    _headers["Content-Type"]   = contentType;
    _ready = true;
}

void HttpResponse::setBodyRaw(const std::string& body)
{
    _body = body;
    _ready = true;
}

void HttpResponse::appendBody(const std::string& chunk)
{
    _body += chunk;
}

std::string HttpResponse::build() const
{
    std::ostringstream ss;
    ss << "HTTP/1.1 " << _statusCode << " " << _statusMsg << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        ss << it->first << ": " << it->second << "\r\n";
    }
    ss << "\r\n";
    ss << _body;
    return ss.str();
}

bool HttpResponse::isReady() const { return _ready; }
int  HttpResponse::getStatusCode() const { return _statusCode; }

std::string HttpResponse::statusMessage(int code)
{
    switch (code) {
        case 100: return "Continue";
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

std::string HttpResponse::mimeType(const std::string& path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css")  return "text/css";
    if (ext == "js")   return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "xml")  return "application/xml";
    if (ext == "txt")  return "text/plain";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png")  return "image/png";
    if (ext == "gif")  return "image/gif";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "pdf")  return "application/pdf";
    if (ext == "zip")  return "application/zip";
    if (ext == "mp4")  return "video/mp4";
    if (ext == "webm") return "video/webm";
    if (ext == "mp3")  return "audio/mpeg";
    return "application/octet-stream";
}

std::string HttpResponse::buildError(int code, const std::string& customPage)
{
    if (!customPage.empty()) return customPage;
    std::string msg = statusMessage(code);
    std::ostringstream body;
    body << "<!DOCTYPE html><html><head><title>"
         << code << " " << msg
         << "</title></head><body><h1>"
         << code << " " << msg
         << "</h1><hr><p>webserv</p></body></html>";
    return body.str();
}
