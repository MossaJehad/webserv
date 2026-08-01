#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>

// Thin wrapper around a single listening socket (interface:port pair).
// Handles creation, SO_REUSEADDR, non-blocking mode, bind and listen.
// It does NOT do accept()/read()/write() by itself - that stays driven
// by the single poll() loop in WebServer.
class Socket {
private:
    int         _fd;
    std::string _host;
    int         _port;

    Socket(const Socket& other);
    Socket& operator=(const Socket& other);

public:
    Socket(const std::string& host, int port);
    ~Socket();

    void        setup();          // socket() + setsockopt + bind + listen + non-blocking
    int         get_fd() const;
    const std::string& get_host() const;
    int         get_port() const;

    static void set_non_blocking(int fd);
};

#endif
