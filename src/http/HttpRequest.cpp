#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
    : _method(METHOD_UNKNOWN),
      _rawMethod(""),
      _uri(""),
      _path(""),
      _query(""),
      _fragment(""),
      _version("HTTP/1.1"),
      _body(""),
      _host(""),
      _port(80),
      _contentLength(0),
      _isChunked(false),
      _keepAlive(true),
      _clientIp(""),
      _clientPort(0) {}

HttpRequest::~HttpRequest() {}

HttpMethod HttpRequest::getMethod() const {
    return _method;
}

void HttpRequest::setMethod(HttpMethod method) {
    _method = method;
}

const std::string& HttpRequest::getRawMethod() const {
    return _rawMethod;
}

void HttpRequest::setRawMethod(const std::string& rawMethod) {
    _rawMethod = rawMethod;
}

const std::string& HttpRequest::getUri() const {
    return _uri;
}

void HttpRequest::setUri(const std::string& uri) {
    _uri = uri;
}

const std::string& HttpRequest::getPath() const {
    return _path;
}

void HttpRequest::setPath(const std::string& path) {
    _path = path;
}

const std::string& HttpRequest::getQuery() const {
    return _query;
}

void HttpRequest::setQuery(const std::string& query) {
    _query = query;
}

const std::string& HttpRequest::getFragment() const {
    return _fragment;
}

void HttpRequest::setFragment(const std::string& fragment) {
    _fragment = fragment;
}

const std::string& HttpRequest::getVersion() const {
    return _version;
}

void HttpRequest::setVersion(const std::string& version) {
    _version = version;
}

const HttpHeaders& HttpRequest::getHeaders() const {
    return _headers;
}

HttpHeaders& HttpRequest::getHeaders() {
    return _headers;
}

const std::string& HttpRequest::getBody() const {
    return _body;
}

void HttpRequest::setBody(const std::string& body) {
    _body = body;
}

void HttpRequest::appendBody(const std::string& chunk) {
    _body.append(chunk);
}

const std::string& HttpRequest::getHost() const {
    return _host;
}

void HttpRequest::setHost(const std::string& host) {
    _host = host;
}

int HttpRequest::getPort() const {
    return _port;
}

void HttpRequest::setPort(int port) {
    _port = port;
}

size_t HttpRequest::getContentLength() const {
    return _contentLength;
}

void HttpRequest::setContentLength(size_t length) {
    _contentLength = length;
}

bool HttpRequest::isChunked() const {
    return _isChunked;
}

void HttpRequest::setIsChunked(bool chunked) {
    _isChunked = chunked;
}

bool HttpRequest::isKeepAlive() const {
    return _keepAlive;
}

void HttpRequest::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}

const std::string& HttpRequest::getClientIp() const {
    return _clientIp;
}

void HttpRequest::setClientIp(const std::string& ip) {
    _clientIp = ip;
}

int HttpRequest::getClientPort() const {
    return _clientPort;
}

void HttpRequest::setClientPort(int port) {
    _clientPort = port;
}

void HttpRequest::clear() {
    _method = METHOD_UNKNOWN;
    _rawMethod.clear();
    _uri.clear();
    _path.clear();
    _query.clear();
    _fragment.clear();
    _version = "HTTP/1.1";
    _headers.clear();
    _body.clear();
    _host.clear();
    _port = 80;
    _contentLength = 0;
    _isChunked = false;
    _keepAlive = true;
    _clientIp.clear();
    _clientPort = 0;
}
