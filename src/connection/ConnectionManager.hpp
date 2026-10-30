#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include "Connection.hpp"
#include "PollRegistry.hpp"
#include "ServerConfig.hpp"
#include <map>
#include <vector>

class ConnectionManager {
private:
    std::map<int, Connection*> _connections;

public:
    ConnectionManager();
    ~ConnectionManager();

    Connection* createConnection(int clientFd,
                                const std::string& clientIp,
                                int clientPort,
                                const std::vector<ServerConfig>& servers,
                                int serverPort,
                                PollRegistry& registry);

    void sweepDeadAndTimedOut(PollRegistry& registry);
    // Returns true when fd identified a client connection that was torn down.
    bool closeConnection(int fd, PollRegistry& registry);
    void clear(PollRegistry& registry);
    size_t count() const;
};

#endif
