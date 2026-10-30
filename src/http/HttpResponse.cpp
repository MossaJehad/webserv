#include "HttpResponse.hpp"
#include "StringUtils.hpp"

HttpResponse::HttpResponse()
    : _statusCode(200),
      _reasonPhrase("OK"),
      _body(""),
      _keepAlive(true),
      _isRaw(false) {}

HttpResponse::HttpResponse(int statusCode)
    : _statusCode(statusCode),
      _reasonPhrase(HttpStatus::getReasonPhrase(statusCode)),
      _body(""),
      _keepAlive(true),
      _isRaw(false) {}

HttpResponse::~HttpResponse() {}

int HttpResponse::getStatusCode() const {
    return _statusCode;
}

void HttpResponse::setStatusCode(int statusCode) {
    _statusCode = statusCode;
    _reasonPhrase = HttpStatus::getReasonPhrase(statusCode);
}

const std::string& HttpResponse::getReasonPhrase() const {
    return _reasonPhrase;
}

void HttpResponse::setReasonPhrase(const std::string& reasonPhrase) {
    _reasonPhrase = reasonPhrase;
}

const HttpHeaders& HttpResponse::getHeaders() const {
    return _headers;
}

HttpHeaders& HttpResponse::getHeaders() {
    return _headers;
}

const std::string& HttpResponse::getBody() const {
    return _body;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    setContentLength(_body.size());
}

void HttpResponse::appendBody(const std::string& data) {
    _body.append(data);
    setContentLength(_body.size());
}

bool HttpResponse::isKeepAlive() const {
    return _keepAlive;
}

void HttpResponse::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}

bool HttpResponse::isRaw() const {
    return _isRaw;
}

void HttpResponse::setIsRaw(bool isRaw) {
    _isRaw = isRaw;
}

void HttpResponse::setContentType(const std::string& mimeType) {
    _headers.set("Content-Type", mimeType);
}

void HttpResponse::setContentLength(size_t length) {
    _headers.set("Content-Length", StringUtils::toString(length));
}

void HttpResponse::setCookie(const std::string& name, const std::string& value, const std::string& path, int maxAge) {
    std::string cookie = name + "=" + value + "; Path=" + path;
    if (maxAge >= 0) {
        cookie += "; Max-Age=" + StringUtils::toString(maxAge);
    }
    _headers.add("Set-Cookie", cookie);
}

HttpResponse HttpResponse::redirect(const std::string& location, int code) {
    HttpResponse res(code);
    res.getHeaders().set("Location", location);
    res.setBody("<html><body><h1>" + StringUtils::toString(code) + " " + HttpStatus::getReasonPhrase(code) +
                "</h1><p>Redirecting to <a href=\"" + location + "\">" + location + "</a></p></body></html>");
    res.setContentType("text/html");
    return res;
}
