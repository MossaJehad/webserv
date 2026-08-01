#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include "Config.hpp"
#include "Socket.hpp"
#include "ClientConnection.hpp"

// Owns every listening socket AND every client socket behind a single
// poll() call, as required by the subject:
//   - 1 poll() for all I/O (listen included)
//   - poll() watches read & write simultaneously
//   - read()/recv()/write()/send() only ever happen right after poll()
//     reported the fd ready for that operation.
class WebServer {
private:
    std::vector<ServerConfig>          _server_configs;
    std::vector<Socket*>               _listeners;

    // one listening fd can serve several server{} blocks that share the
    // same host:port (kept for the future router; unused for routing yet)
    std::map<int, std::vector<const ServerConfig*> > _listen_fd_to_configs;

    std::map<int, ClientConnection>    _clients;
    std::vector<struct pollfd>         _poll_fds;

    bool _running;

    WebServer(const WebServer& other);
    WebServer& operator=(const WebServer& other);

    void setup_listeners();
    void rebuild_poll_fds();
    void accept_new_connection(int listen_fd);
    void handle_client_event(int fd, short revents);
    void close_client(int fd);

public:
    explicit WebServer(const std::vector<ServerConfig>& server_configs);
    ~WebServer();

    void run();
    void stop();
};

#endif
