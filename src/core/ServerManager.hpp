#pragma once
#include <vector>
#include <map>
#include <poll.h>
#include "../config/ServerConfig.hpp"
#include "Client.hpp"

class ServerManager {
public:
    ServerManager();
    ~ServerManager();

    bool init(const std::vector<ServerConfig>& servers);
    void run();

private:
    std::vector<ServerConfig> _servers;

    // listen fd -> index in _servers (first matching server for that port)
    std::map<int, int> _listenFdToServer;
    // All listen fds in port-order; multiple servers may share a port
    std::vector<int> _listenFds;

    // client fd -> Client*
    std::map<int, Client*> _clients;

    std::vector<struct pollfd> _pollfds;

    bool createListenSocket(const ServerConfig& srv, int srvIdx);
    void acceptClient(int listenFd);
    void handleClient(int fd, short revents);
    void processRequest(Client& client);
    void removeClient(int fd);

    // Match virtual host from Host header
    const ServerConfig* matchServer(const std::string& host, int listenFd) const;

    void setNonBlocking(int fd);
    static const int TIMEOUT_SEC = 60;
    static const int MAX_CLIENTS = 1024;
};
