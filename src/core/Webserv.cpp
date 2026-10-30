#include "Webserv.hpp"
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <map>
#include <set>

Webserv::Webserv() : _reactor(_registry, _connManager) {}

Webserv::~Webserv() {
    stop();
}

bool Webserv::init(const std::string& configFilePath) {
    Logger::info("Loading configuration from " + configFilePath);

    try {
        ConfigParser parser;
        _servers = parser.parse(configFilePath);
        ConfigValidator::validate(_servers);
    } catch (const std::exception& e) {
        Logger::error("Configuration error: " + std::string(e.what()));
        return false;
    }

    // Map unique (host, port) -> list of ServerConfigs
    std::map<std::pair<std::string, int>, std::vector<ServerConfig> > listenMap;

    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig& srv = _servers[i];
        const std::vector<int>& ports = srv.getPorts();
        for (size_t p = 0; p < ports.size(); ++p) {
            std::pair<std::string, int> endpoint = std::make_pair(srv.getHost(), ports[p]);
            listenMap[endpoint].push_back(srv);
        }
    }

    for (std::map<std::pair<std::string, int>, std::vector<ServerConfig> >::iterator it = listenMap.begin();
         it != listenMap.end(); ++it) {
        std::string host = it->first.first;
        int port = it->first.second;
        const std::vector<ServerConfig>& srvList = it->second;

        Listener* listener = new Listener(host, port, srvList, _connManager, _registry);
        if (!listener->init()) {
            Logger::error("Failed to initialize listener on " + host + ":" + StringUtils::toString(port));
            delete listener;
            return false;
        }

        _listeners.push_back(listener);
        _registry.registerHandler(listener);
    }

    if (_listeners.empty()) {
        Logger::error("No listeners could be established");
        return false;
    }

    return true;
}

void Webserv::run() {
    _reactor.run();
}

void Webserv::stop() {
    _reactor.stop();

    for (size_t i = 0; i < _listeners.size(); ++i) {
        _registry.unregisterHandler(_listeners[i]);
        delete _listeners[i];
    }
    _listeners.clear();

    _connManager.clear(_registry);
    _registry.clear();
}
