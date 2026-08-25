#include "Listener.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

Listener::Listener(const std::string& host,
                   int port,
                   const std::vector<ServerConfig>& servers,
                   ConnectionManager& connManager,
                   PollRegistry& registry)
    : _host(host),
      _port(port),
      _servers(servers),
      _connManager(&connManager),
      _registry(&registry) {}

Listener::~Listener() {}

bool Listener::init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Logger::error("Failed to create socket for " + _host + ":" + StringUtils::toString(_port));
        return false;
    }

    _socket.setFd(fd);

    if (!Socket::setReuseAddr(fd)) {
        Logger::warn("Failed to set SO_REUSEADDR on " + _host + ":" + StringUtils::toString(_port));
    }

    if (!Socket::setNonBlocking(fd)) {
        Logger::error("Failed to set non-blocking on listener socket");
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);

    if (_host.empty() || _host == "0.0.0.0" || _host == "*") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        addr.sin_addr.s_addr = inet_addr(_host.c_str());
        if (addr.sin_addr.s_addr == INADDR_NONE) {
            struct hostent* he = gethostbyname(_host.c_str());
            if (he && he->h_addr_list[0]) {
                std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
            } else {
                addr.sin_addr.s_addr = htonl(INADDR_ANY);
            }
        }
    }

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("Failed to bind on " + _host + ":" + StringUtils::toString(_port));
        return false;
    }

    if (::listen(fd, 128) < 0) {
        Logger::error("Failed to listen on " + _host + ":" + StringUtils::toString(_port));
        return false;
    }

    Logger::info("Listening on http://" + _host + ":" + StringUtils::toString(_port));
    return true;
}

int Listener::getFd() const {
    return _socket.getFd();
}

void Listener::handleRead() {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        std::memset(&clientAddr, 0, sizeof(clientAddr));

        int clientFd = accept(_socket.getFd(), (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            break; // No more incoming connections
        }

        std::string clientIp = inet_ntoa(clientAddr.sin_addr);
        int clientPort = ntohs(clientAddr.sin_port);

        if (_connManager && _registry) {
            _connManager->createConnection(clientFd, clientIp, clientPort, _servers, _port, *_registry);
        } else {
            close(clientFd);
        }
    }
}

void Listener::handleWrite() {}

bool Listener::wantsRead() const {
    return _socket.getFd() >= 0;
}

bool Listener::wantsWrite() const {
    return false;
}

bool Listener::isDead() const {
    return _socket.getFd() < 0;
}

const std::string& Listener::getHost() const {
    return _host;
}

int Listener::getPort() const {
    return _port;
}
