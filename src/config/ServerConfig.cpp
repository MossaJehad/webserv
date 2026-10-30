#include "ServerConfig.hpp"
#include "StringUtils.hpp"
#include <algorithm>

ServerConfig::ServerConfig()
    : _host(DEFAULT_HOST),
      _clientMaxBodySize(DEFAULT_CLIENT_MAX_BODY_SIZE),
      _root(DEFAULT_ROOT),
      _index(DEFAULT_INDEX) {}

ServerConfig::~ServerConfig() {}

const std::string& ServerConfig::getHost() const {
    return _host;
}

void ServerConfig::setHost(const std::string& host) {
    _host = host;
}

const std::vector<int>& ServerConfig::getPorts() const {
    return _ports;
}

void ServerConfig::addPort(int port) {
    if (std::find(_ports.begin(), _ports.end(), port) == _ports.end()) {
        _ports.push_back(port);
    }
}

bool ServerConfig::listensOnPort(int port) const {
    return std::find(_ports.begin(), _ports.end(), port) != _ports.end();
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
    return _serverNames;
}

void ServerConfig::addServerName(const std::string& name) {
    if (std::find(_serverNames.begin(), _serverNames.end(), name) == _serverNames.end()) {
        _serverNames.push_back(name);
    }
}

bool ServerConfig::matchesServerName(const std::string& name) const {
    std::string lowerName = StringUtils::toLower(name);
    for (size_t i = 0; i < _serverNames.size(); ++i) {
        if (StringUtils::toLower(_serverNames[i]) == lowerName) {
            return true;
        }
    }
    return false;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return _errorPages;
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
    _errorPages[code] = path;
}

std::string ServerConfig::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = _errorPages.find(code);
    if (it != _errorPages.end()) {
        return it->second;
    }
    return "";
}

size_t ServerConfig::getClientMaxBodySize() const {
    return _clientMaxBodySize;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
    _clientMaxBodySize = size;
}

const std::string& ServerConfig::getRoot() const {
    return _root;
}

void ServerConfig::setRoot(const std::string& root) {
    _root = root;
}

const std::string& ServerConfig::getIndex() const {
    return _index;
}

void ServerConfig::setIndex(const std::string& index) {
    _index = index;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return _locations;
}

std::vector<LocationConfig>& ServerConfig::getLocations() {
    return _locations;
}

void ServerConfig::addLocation(const LocationConfig& location) {
    _locations.push_back(location);
}
