#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : _path(""),
      _root(""),
      _index(""),
      _autoindex(false),
      _allowedMethods(0),
      _redirectCode(0),
      _redirectUrl(""),
      _uploadDir(""),
      _clientMaxBodySize(0),
      _hasBodySizeLimit(false) {}

LocationConfig::~LocationConfig() {}

const std::string& LocationConfig::getPath() const {
    return _path;
}

void LocationConfig::setPath(const std::string& path) {
    _path = path;
}

const std::string& LocationConfig::getRoot() const {
    return _root;
}

void LocationConfig::setRoot(const std::string& root) {
    _root = root;
}

const std::string& LocationConfig::getIndex() const {
    return _index;
}

void LocationConfig::setIndex(const std::string& index) {
    _index = index;
}

bool LocationConfig::getAutoindex() const {
    return _autoindex;
}

void LocationConfig::setAutoindex(bool autoindex) {
    _autoindex = autoindex;
}

int LocationConfig::getAllowedMethods() const {
    return _allowedMethods;
}

void LocationConfig::setAllowedMethods(int methods) {
    _allowedMethods = methods;
}

void LocationConfig::addAllowedMethod(HttpMethod method) {
    _allowedMethods |= method;
}

bool LocationConfig::isMethodAllowed(HttpMethod method) const {
    if (_allowedMethods == 0) {
        return method == METHOD_GET; // default to GET if none specified
    }
    return (_allowedMethods & method) != 0;
}

int LocationConfig::getRedirectCode() const {
    return _redirectCode;
}

const std::string& LocationConfig::getRedirectUrl() const {
    return _redirectUrl;
}

void LocationConfig::setRedirect(int code, const std::string& url) {
    _redirectCode = code;
    _redirectUrl = url;
}

bool LocationConfig::hasRedirect() const {
    return _redirectCode > 0 && !_redirectUrl.empty();
}

const std::string& LocationConfig::getUploadDir() const {
    return _uploadDir;
}

void LocationConfig::setUploadDir(const std::string& uploadDir) {
    _uploadDir = uploadDir;
}

bool LocationConfig::allowsUpload() const {
    return !_uploadDir.empty();
}

const std::map<std::string, std::string>& LocationConfig::getCgiPass() const {
    return _cgiPass;
}

void LocationConfig::addCgiHandler(const std::string& ext, const std::string& binPath) {
    _cgiPass[ext] = binPath;
}

bool LocationConfig::isCgiExtension(const std::string& ext) const {
    return _cgiPass.find(ext) != _cgiPass.end();
}

std::string LocationConfig::getCgiBin(const std::string& ext) const {
    std::map<std::string, std::string>::const_iterator it = _cgiPass.find(ext);
    if (it != _cgiPass.end()) {
        return it->second;
    }
    return "";
}

size_t LocationConfig::getClientMaxBodySize() const {
    return _clientMaxBodySize;
}

void LocationConfig::setClientMaxBodySize(size_t size) {
    _clientMaxBodySize = size;
    _hasBodySizeLimit = true;
}

bool LocationConfig::hasBodySizeLimit() const {
    return _hasBodySizeLimit;
}
