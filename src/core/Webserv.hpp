#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "ServerConfig.hpp"
#include "PollRegistry.hpp"
#include "ConnectionManager.hpp"
#include "Listener.hpp"
#include "Reactor.hpp"
#include <string>
#include <vector>

class Webserv {
private:
    std::vector<ServerConfig> _servers;
    PollRegistry _registry;
    ConnectionManager _connManager;
    std::vector<Listener*> _listeners;
    Reactor _reactor;

    // Non-copyable
    Webserv(const Webserv&);
    Webserv& operator=(const Webserv&);

public:
    Webserv();
    ~Webserv();

    bool init(const std::string& configFilePath);
    void run();
    void stop();
};

#endif
