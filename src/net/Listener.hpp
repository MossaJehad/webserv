#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "IEventHandler.hpp"
#include "Socket.hpp"
#include "ServerConfig.hpp"
#include "ConnectionManager.hpp"
#include "PollRegistry.hpp"
#include <string>
#include <vector>

class Listener : public IEventHandler {
private:
    Socket _socket;
    std::string _host;
    int _port;
    std::vector<ServerConfig> _servers;
    ConnectionManager* _connManager;
    PollRegistry* _registry;

public:
    Listener(const std::string& host,
             int port,
             const std::vector<ServerConfig>& servers,
             ConnectionManager& connManager,
             PollRegistry& registry);
    virtual ~Listener();

    bool init();

    virtual int getFd() const;
    virtual void handleRead();
    virtual void handleWrite();
    virtual bool wantsRead() const;
    virtual bool wantsWrite() const;
    virtual bool isDead() const;

    const std::string& getHost() const;
    int getPort() const;
};

#endif
