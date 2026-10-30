#include "RequestContext.hpp"

RequestContext::RequestContext()
    : _server(NULL),
      _resolvedFsPath(""),
      _scriptPath(""),
      _pathInfo(""),
      _isCgi(false),
      _cgiBin("") {}

RequestContext::~RequestContext() {}

const HttpRequest& RequestContext::getRequest() const {
    return _request;
}

HttpRequest& RequestContext::getRequest() {
    return _request;
}

void RequestContext::setRequest(const HttpRequest& req) {
    _request = req;
}

const ServerConfig* RequestContext::getServer() const {
    return _server;
}

void RequestContext::setServer(const ServerConfig* server) {
    _server = server;
}

const LocationConfig& RequestContext::getLocation() const {
    return _location;
}

LocationConfig& RequestContext::getLocation() {
    return _location;
}

void RequestContext::setLocation(const LocationConfig& loc) {
    _location = loc;
}

const std::string& RequestContext::getResolvedFsPath() const {
    return _resolvedFsPath;
}

void RequestContext::setResolvedFsPath(const std::string& path) {
    _resolvedFsPath = path;
}

const std::string& RequestContext::getScriptPath() const {
    return _scriptPath;
}

void RequestContext::setScriptPath(const std::string& scriptPath) {
    _scriptPath = scriptPath;
}

const std::string& RequestContext::getPathInfo() const {
    return _pathInfo;
}

void RequestContext::setPathInfo(const std::string& pathInfo) {
    _pathInfo = pathInfo;
}

bool RequestContext::isCgi() const {
    return _isCgi;
}

void RequestContext::setIsCgi(bool isCgi) {
    _isCgi = isCgi;
}

const std::string& RequestContext::getCgiBin() const {
    return _cgiBin;
}

void RequestContext::setCgiBin(const std::string& cgiBin) {
    _cgiBin = cgiBin;
}
