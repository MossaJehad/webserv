#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "ConfigTypes.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <vector>
#include <map>

class ServerConfig {
private:
    std::string _host;
    std::vector<int> _ports;
    std::vector<std::string> _serverNames;
    std::map<int, std::string> _errorPages;
    size_t _clientMaxBodySize;
    std::string _root;
    std::string _index;
    std::vector<LocationConfig> _locations;

public:
    ServerConfig();
    ~ServerConfig();

    const std::string& getHost() const;
    void setHost(const std::string& host);

    const std::vector<int>& getPorts() const;
    void addPort(int port);
    bool listensOnPort(int port) const;

    const std::vector<std::string>& getServerNames() const;
    void addServerName(const std::string& name);
    bool matchesServerName(const std::string& name) const;

    const std::map<int, std::string>& getErrorPages() const;
    void addErrorPage(int code, const std::string& path);
    std::string getErrorPage(int code) const;

    size_t getClientMaxBodySize() const;
    void setClientMaxBodySize(size_t size);

    const std::string& getRoot() const;
    void setRoot(const std::string& root);

    const std::string& getIndex() const;
    void setIndex(const std::string& index);

    const std::vector<LocationConfig>& getLocations() const;
    std::vector<LocationConfig>& getLocations();
    void addLocation(const LocationConfig& location);
};

#endif
