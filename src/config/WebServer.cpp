#include "WebServer.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <sstream>

static volatile sig_atomic_t g_stop_requested = 0;

static void handle_signal(int)
{
    g_stop_requested = 1;
}

WebServer::WebServer(const std::vector<ServerConfig>& server_configs)
    : _server_configs(server_configs), _running(false)
{
    setup_listeners();
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGPIPE, SIG_IGN); // a send() to a half-closed peer must not kill us
}

WebServer::~WebServer()
{
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        close(it->first);

    for (size_t i = 0; i < _listeners.size(); ++i)
        delete _listeners[i];
}

// Groups server{} blocks that share the same host:port under one real
// listening socket - this is what lets one program serve multiple
// "websites" (multiple server{} entries) on the ports the config asks for.
void WebServer::setup_listeners()
{
    std::map<std::string, int> key_to_fd; // "host:port" -> already-created listen fd

    for (size_t i = 0; i < _server_configs.size(); ++i)
    {
        const ServerConfig& cfg = _server_configs[i];

        std::ostringstream key_stream;
        key_stream << cfg.host << ":" << cfg.listen_port;
        std::string key = key_stream.str();

        std::map<std::string, int>::iterator found = key_to_fd.find(key);
        if (found != key_to_fd.end())
        {
            _listen_fd_to_configs[found->second].push_back(&_server_configs[i]);
            continue;
        }

        Socket* sock = new Socket(cfg.host, cfg.listen_port);
        try
        {
            sock->setup();
        }
        catch (const std::exception& e)
        {
            delete sock;
            throw;
        }

        int fd = sock->get_fd();
        _listeners.push_back(sock);
        key_to_fd[key] = fd;
        _listen_fd_to_configs[fd].push_back(&_server_configs[i]);

        std::cout << "Listening on " << cfg.host << ":" << cfg.listen_port << std::endl;
    }
}

void WebServer::rebuild_poll_fds()
{
    _poll_fds.clear();

    for (size_t i = 0; i < _listeners.size(); ++i)
    {
        struct pollfd pfd;
        pfd.fd = _listeners[i]->get_fd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }

    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        struct pollfd pfd;
        pfd.fd = it->first;
        pfd.events = 0;
        if (it->second.wants_to_read())
            pfd.events |= POLLIN;
        if (it->second.wants_to_write())
            pfd.events |= POLLOUT;
        pfd.revents = 0;
        _poll_fds.push_back(pfd);
    }
}

void WebServer::accept_new_connection(int listen_fd)
{
    // One accept() per ready event: poll() is level-triggered, so if more
    // connections are queued it will report POLLIN again next iteration.
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd == -1)
        return; // nothing to accept right now, or a transient failure

    Socket::set_non_blocking(client_fd);

    const std::vector<const ServerConfig*>& candidates = _listen_fd_to_configs[listen_fd];
    _clients[client_fd] = ClientConnection(client_fd, listen_fd, candidates);
}

void WebServer::close_client(int fd)
{
    close(fd);
    _clients.erase(fd);
}

void WebServer::handle_client_event(int fd, short revents)
{
    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    ClientConnection& conn = it->second;

    if (revents & (POLLHUP | POLLERR | POLLNVAL))
    {
        close_client(fd);
        return;
    }

    if (revents & POLLIN)
    {
        if (conn.on_readable())
        {
            close_client(fd);
            return;
        }
    }

    if (revents & POLLOUT)
    {
        if (conn.on_writable())
        {
            close_client(fd);
            return;
        }
    }

    if (conn.should_close())
        close_client(fd);
}

void WebServer::run()
{
    _running = true;

    while (_running && !g_stop_requested)
    {
        rebuild_poll_fds();

        int ready = poll(&_poll_fds[0], _poll_fds.size(), -1);
        if (ready == -1)
        {
            if (g_stop_requested)
                break;
            continue; // interrupted by a signal we don't care about, just re-poll
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i)
        {
            if (_poll_fds[i].revents == 0)
                continue;

            bool is_listener = false;
            for (size_t j = 0; j < _listeners.size(); ++j)
            {
                if (_listeners[j]->get_fd() == _poll_fds[i].fd)
                {
                    is_listener = true;
                    if (_poll_fds[i].revents & POLLIN)
                        accept_new_connection(_poll_fds[i].fd);
                    break;
                }
            }

            if (!is_listener)
                handle_client_event(_poll_fds[i].fd, _poll_fds[i].revents);
        }
    }

    std::cout << "webserv: shutting down." << std::endl;
}

void WebServer::stop()
{
    _running = false;
}
