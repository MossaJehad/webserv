#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

class Socket {
private:
    int _fd;

    // Non-copyable
    Socket(const Socket&);
    Socket& operator=(const Socket&);

public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    int getFd() const;
    void setFd(int fd);
    int release();
    void close();

    static bool setNonBlocking(int fd);
    static bool setReuseAddr(int fd);
    static bool setCloexec(int fd);
};

#endif
