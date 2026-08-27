#include "Listener.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>

namespace {

// Resolve a host string ("127.0.0.1", "localhost", "") to an IPv4 address using
// getaddrinfo(), which is the resolver allowed by the subject.
bool resolveIpv4(const std::string& host, struct in_addr& out) {
    if (host.empty() || host == "0.0.0.0" || host == "*") {
        out.s_addr = htonl(INADDR_ANY);
        return true;
    }

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = NULL;
    int rc = getaddrinfo(host.c_str(), NULL, &hints, &res);
    if (rc != 0 || !res) {
        Logger::error("Cannot resolve host '" + host + "': " + std::string(gai_strerror(rc)));
        return false;
    }

    const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(res->ai_addr);
    out = sin->sin_addr;
    freeaddrinfo(res);
    return true;
}

// Render an IPv4 address as dotted quad without relying on inet_ntoa().
std::string formatIpv4(const struct in_addr& addr) {
    unsigned long host = static_cast<unsigned long>(ntohl(addr.s_addr));
    std::string out;
    for (int shift = 24; shift >= 0; shift -= 8) {
        out += StringUtils::toString(static_cast<size_t>((host >> shift) & 0xFFUL));
        if (shift > 0) {
            out += ".";
        }
    }
    return out;
}

} // namespace

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
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);

    if (!resolveIpv4(_host, addr.sin_addr)) {
        return false;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        Logger::error("Failed to create socket for " + _host + ":" + StringUtils::toString(_port) + ": " + std::strerror(errno));
        return false;
    }

    _socket.setFd(fd);

    if (!Socket::setReuseAddr(fd)) {
        Logger::warn("Failed to set SO_REUSEADDR on " + _host + ":" + StringUtils::toString(_port));
    }

    if (!Socket::setNonBlocking(fd)) {
        Logger::error("Failed to set non-blocking on listener socket: " + std::string(std::strerror(errno)));
        return false;
    }

    // Keep the listening socket out of forked CGI children: an orphaned child
    // would otherwise hold the port bound after the server exits.
    Socket::setCloexec(fd);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("Failed to bind on " + _host + ":" + StringUtils::toString(_port) + ": " + std::strerror(errno));
        return false;
    }

    if (::listen(fd, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on " + _host + ":" + StringUtils::toString(_port) + ": " + std::strerror(errno));
        return false;
    }

    Logger::info("Listening on http://" + _host + ":" + StringUtils::toString(_port));
    return true;
}

int Listener::getFd() const {
    return _socket.getFd();
}

void Listener::handleRead() {
    // Accept exactly one connection per readiness notification. poll() is
    // level-triggered, so any remaining backlog is reported again next cycle.
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    std::memset(&clientAddr, 0, sizeof(clientAddr));

    int clientFd = accept(_socket.getFd(), (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
        return; // Nothing pending, or the pending connection was dropped
    }

    if (!_connManager || !_registry) {
        close(clientFd);
        return;
    }

    _connManager->createConnection(clientFd,
                                   formatIpv4(clientAddr.sin_addr),
                                   ntohs(clientAddr.sin_port),
                                   _servers,
                                   _port,
                                   *_registry);
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
