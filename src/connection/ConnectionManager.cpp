#include "ConnectionManager.hpp"
#include "Time.hpp"

ConnectionManager::ConnectionManager() {}

ConnectionManager::~ConnectionManager() {
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        delete it->second;
    }
    _connections.clear();
}

Connection* ConnectionManager::createConnection(int clientFd,
                                                const std::string& clientIp,
                                                int clientPort,
                                                const std::vector<ServerConfig>& servers,
                                                int serverPort,
                                                PollRegistry& registry) {
    Connection* conn = new Connection(clientFd, clientIp, clientPort, servers, serverPort, registry);
    _connections[clientFd] = conn;
    registry.registerHandler(conn);
    return conn;
}

void ConnectionManager::sweepDeadAndTimedOut(PollRegistry& registry) {
    std::time_t now = Time::now();
    std::vector<int> toRemove;

    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        Connection* conn = it->second;
        if (!conn) {
            toRemove.push_back(it->first);
            continue;
        }

        if (conn->isDead()) {
            toRemove.push_back(it->first);
            continue;
        }

        if (conn->isTimedOut(now)) {
            conn->handleTimeout();
            if (conn->isDead()) {
                toRemove.push_back(it->first);
            }
        }
    }

    for (size_t i = 0; i < toRemove.size(); ++i) {
        closeConnection(toRemove[i], registry);
    }
}

void ConnectionManager::closeConnection(int fd, PollRegistry& registry) {
    std::map<int, Connection*>::iterator it = _connections.find(fd);
    if (it != _connections.end()) {
        registry.unregisterHandler(fd);
        delete it->second;
        _connections.erase(it);
    }
}

void ConnectionManager::clear(PollRegistry& registry) {
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        registry.unregisterHandler(it->first);
        delete it->second;
    }
    _connections.clear();
}

size_t ConnectionManager::count() const {
    return _connections.size();
}
