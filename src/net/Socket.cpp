#include "Socket.hpp"

Socket::Socket() : _fd(-1) {}

Socket::Socket(int fd) : _fd(fd) {}

Socket::~Socket() {
    close();
}

int Socket::getFd() const {
    return _fd;
}

void Socket::setFd(int fd) {
    close();
    _fd = fd;
}

int Socket::release() {
    int temp = _fd;
    _fd = -1;
    return temp;
}

void Socket::close() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

bool Socket::setNonBlocking(int fd) {
    if (fd < 0) return false;
    return fcntl(fd, F_SETFL, O_NONBLOCK) >= 0;
}

bool Socket::setReuseAddr(int fd) {
    if (fd < 0) return false;
    int opt = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) >= 0;
}

bool Socket::setCloexec(int fd) {
    if (fd < 0) return false;
    return fcntl(fd, F_SETFD, FD_CLOEXEC) >= 0;
}
