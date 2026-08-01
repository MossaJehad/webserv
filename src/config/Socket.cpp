#include "Socket.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <cerrno>

Socket::Socket(const std::string& host, int port)
    : _fd(-1), _host(host), _port(port)
{
}

Socket::~Socket()
{
    if (_fd != -1)
        close(_fd);
}

int Socket::get_fd() const
{
    return _fd;
}

const std::string& Socket::get_host() const
{
    return _host;
}

int Socket::get_port() const
{
    return _port;
}

void Socket::set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("fcntl(F_GETFL) failed: " + std::string(strerror(errno)));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK) failed: " + std::string(strerror(errno)));
}

void Socket::setup()
{
    struct addrinfo hints;
    struct addrinfo* res;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    std::ostringstream port_stream;
    port_stream << _port;

    const char* node = (_host.empty() || _host == "0.0.0.0") ? NULL : _host.c_str();

    int gai_ret = getaddrinfo(node, port_stream.str().c_str(), &hints, &res);
    if (gai_ret != 0)
        throw std::runtime_error("getaddrinfo failed for " + _host + ":" + port_stream.str()
                                  + " -> " + std::string(gai_strerror(gai_ret)));

    _fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_fd == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
    }

    if (bind(_fd, res->ai_addr, res->ai_addrlen) == -1) {
        std::string err = strerror(errno);
        freeaddrinfo(res);
        throw std::runtime_error("bind() failed on " + _host + ":" + port_stream.str() + " -> " + err);
    }
    freeaddrinfo(res);

    if (listen(_fd, SOMAXCONN) == -1)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));

    set_non_blocking(_fd);
}
