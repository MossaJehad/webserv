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
    // The kernel reuses descriptor numbers, so make sure no stale connection is
    // still parked on this fd before taking it over.
    std::map<int, Connection*>::iterator stale = _connections.find(clientFd);
    if (stale != _connections.end()) {
        registry.unregisterHandler(clientFd, stale->second);
        delete stale->second;
        _connections.erase(stale);
    }

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

        if (conn->getState() == CONN_STATE_WAIT_CGI) {
            conn->checkCgi();
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
        // Identity-checked: by now this connection's socket is closed and the
        // number may already belong to a CGI pipe.
        registry.unregisterHandler(fd, it->second);
        delete it->second;
        _connections.erase(it);
    }
}

void ConnectionManager::clear(PollRegistry& registry) {
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        registry.unregisterHandler(it->first, it->second);
        delete it->second;
    }
    _connections.clear();
}

size_t ConnectionManager::count() const {
    return _connections.size();
}
