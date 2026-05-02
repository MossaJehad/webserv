#include "ServerManager.hpp"
#include "../http/Router.hpp"
#include "../handlers/StaticHandler.hpp"
#include "../handlers/UploadHandler.hpp"
#include "../handlers/CgiHandler.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
    for (std::map<int, Client*>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        delete it->second;
    }
    for (size_t i = 0; i < _listenFds.size(); ++i)
        close(_listenFds[i]);
}

void ServerManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool ServerManager::init(const std::vector<ServerConfig>& servers)
{
    _servers = servers;

    // Track (host:port) already bound so we share listen sockets
    std::map<std::string, int> boundPorts;

    for (size_t i = 0; i < _servers.size(); ++i) {
        std::ostringstream key;
        key << _servers[i].host << ":" << _servers[i].port;
        std::string keyStr = key.str();

        if (boundPorts.find(keyStr) != boundPorts.end()) {
            // Multiple server blocks on same port — share listen fd
            // The first server is default; virtual host matching happens in matchServer
            continue;
        }

        if (!createListenSocket(_servers[i], static_cast<int>(i)))
            return false;
        boundPorts[keyStr] = _servers[i].port;
    }
    return true;
}

bool ServerManager::createListenSocket(const ServerConfig& srv, int srvIdx)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setNonBlocking(fd);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(srv.port));
    if (srv.host == "0.0.0.0" || srv.host.empty())
        addr.sin_addr.s_addr = INADDR_ANY;
    else
        addr.sin_addr.s_addr = inet_addr(srv.host.c_str());

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed on port " << srv.port
                  << ": " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    std::cout << "Listening on " << (srv.host.empty() ? "0.0.0.0" : srv.host)
              << ":" << srv.port << std::endl;

    _listenFds.push_back(fd);
    _listenFdToServer[fd] = srvIdx;

    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    return true;
}

void ServerManager::run()
{
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;

    while (true) {
        // Rebuild pollfd vector
        _pollfds.clear();
        for (size_t i = 0; i < _listenFds.size(); ++i) {
            struct pollfd pfd;
            pfd.fd      = _listenFds[i];
            pfd.events  = POLLIN;
            pfd.revents = 0;
            _pollfds.push_back(pfd);
        }
        for (std::map<int, Client*>::iterator it = _clients.begin();
             it != _clients.end(); ++it) {
            struct pollfd pfd;
            pfd.fd      = it->first;
            pfd.events  = 0;
            pfd.revents = 0;
            if (it->second->getState() == CS_READING)
                pfd.events |= POLLIN;
            if (it->second->getState() == CS_SENDING)
                pfd.events |= POLLOUT;
            _pollfds.push_back(pfd);
        }

        int nReady = poll(_pollfds.data(), static_cast<nfds_t>(_pollfds.size()), 5000);
        if (nReady < 0) {
            if (errno == EINTR) continue;
            std::cerr << "poll() error: " << strerror(errno) << std::endl;
            break;
        }

        // Timeout — reap stale connections
        if (nReady == 0) {
            time_t now = time(NULL);
            std::vector<int> toRemove;
            for (std::map<int, Client*>::iterator it = _clients.begin();
                 it != _clients.end(); ++it) {
                if (now - it->second->getLastActivity() > TIMEOUT_SEC)
                    toRemove.push_back(it->first);
            }
            for (size_t i = 0; i < toRemove.size(); ++i)
                removeClient(toRemove[i]);
            continue;
        }

        for (size_t i = 0; i < _pollfds.size() && nReady > 0; ++i) {
            if (_pollfds[i].revents == 0) continue;
            --nReady;

            int fd = _pollfds[i].fd;

            // Is it a listen socket?
            bool isListen = false;
            for (size_t j = 0; j < _listenFds.size(); ++j) {
                if (_listenFds[j] == fd) { isListen = true; break; }
            }

            if (isListen) {
                if (_pollfds[i].revents & POLLIN)
                    acceptClient(fd);
            } else {
                handleClient(fd, _pollfds[i].revents);
            }
        }
    }
}

void ServerManager::acceptClient(int listenFd)
{
    if ((int)_clients.size() >= MAX_CLIENTS) return;

    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    int clientFd = accept(listenFd, (struct sockaddr*)&addr, &addrLen);
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
        return;
    }

    setNonBlocking(clientFd);

    std::string ip = inet_ntoa(addr.sin_addr);

    int srvIdx = _listenFdToServer[listenFd];
    const ServerConfig* srv = &_servers[static_cast<size_t>(srvIdx)];

    Client* client = new Client(clientFd, ip, srv);
    _clients[clientFd] = client;
}

const ServerConfig* ServerManager::matchServer(const std::string& host,
                                                int listenFd) const
{
    int defaultIdx = -1;
    std::map<int, int>::const_iterator dit = _listenFdToServer.find(listenFd);
    if (dit != _listenFdToServer.end())
        defaultIdx = dit->second;

    int port = -1;
    for (size_t i = 0; i < _listenFds.size(); ++i) {
        if (_listenFds[i] == listenFd) {
            // find port from servers
            if (defaultIdx >= 0)
                port = _servers[static_cast<size_t>(defaultIdx)].port;
            break;
        }
    }

    // Try to match by server_name
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (port >= 0 && _servers[i].port != port) continue;
        for (size_t j = 0; j < _servers[i].serverNames.size(); ++j) {
            if (_servers[i].serverNames[j] == host)
                return &_servers[i];
        }
    }

    // Fall back to default
    if (defaultIdx >= 0)
        return &_servers[static_cast<size_t>(defaultIdx)];
    return _servers.empty() ? NULL : &_servers[0];
}

void ServerManager::handleClient(int fd, short revents)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;

    Client* client = it->second;

    if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
        removeClient(fd);
        return;
    }

    if (revents & POLLIN) {
        if (!client->readData()) {
            removeClient(fd);
            return;
        }

        HttpRequest& req = client->getRequest();

        if (req.hasError()) {
            int code = req.getErrorCode() ? req.getErrorCode() : 400;
            client->getResponse().setStatus(code);
            client->getResponse().setBody(HttpResponse::buildError(code), "text/html");
            client->setState(CS_SENDING);
            return;
        }

        if (req.isComplete()) {
            // Match virtual host
            std::string hostHeader = req.getHeader("host");
            // Strip port from host header
            size_t colon = hostHeader.find(':');
            if (colon != std::string::npos)
                hostHeader = hostHeader.substr(0, colon);

            // Find which listen fd this client came in on
            // (we stored default server at accept time)
            const ServerConfig* srv = client->getServerConfig();
            // Try to refine via virtual host matching
            for (size_t i = 0; i < _listenFds.size(); ++i) {
                std::map<int, int>::iterator sit = _listenFdToServer.find(_listenFds[i]);
                if (sit != _listenFdToServer.end()) {
                    const ServerConfig* candidate = matchServer(hostHeader, _listenFds[i]);
                    if (candidate && candidate->port == srv->port) {
                        srv = candidate;
                        break;
                    }
                }
            }
            client->setServerConfig(srv);

            client->setState(CS_PROCESSING);
            processRequest(*client);
        }
    }

    if (revents & POLLOUT) {
        if (client->getState() == CS_SENDING) {
            if (!client->writeData()) {
                removeClient(fd);
                return;
            }
            if (client->isSendDone()) {
                if (client->isKeepAlive()) {
                    client->reset();
                } else {
                    removeClient(fd);
                }
            }
            // else: partial send — poll will fire POLLOUT again
        }
    }
}

// Determine if path is a CGI script
static bool isCgiPath(const LocationConfig& loc, const std::string& path)
{
    if (loc.cgiExtensions.empty()) return false;
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot + 1);
    for (size_t i = 0; i < loc.cgiExtensions.size(); ++i) {
        std::string locExt = loc.cgiExtensions[i];
        // Strip leading dot if present
        if (!locExt.empty() && locExt[0] == '.')
            locExt = locExt.substr(1);
        if (locExt == ext) return true;
    }
    return false;
}

void ServerManager::processRequest(Client& client)
{
    const ServerConfig* srv = client.getServerConfig();
    HttpRequest& req   = client.getRequest();
    HttpResponse& resp = client.getResponse();

    Router router;
    const LocationConfig* loc = router.route(*srv, req, resp);
    if (!loc) {
        // Response already set by router (404/redirect)
        client.setState(CS_SENDING);
        return;
    }

    if (!router.checkMethod(*loc, req.getMethod(), resp)) {
        client.setState(CS_SENDING);
        return;
    }

    if (!router.checkBodySize(*srv, *loc, req, resp)) {
        client.setState(CS_SENDING);
        return;
    }

    const std::string& method = req.getMethod();

    // Resolve full script path for CGI check
    // nginx `root` semantics: root + full_uri_path (no stripping of location prefix)
    std::string root = loc->root.empty() ? "./www/html" : loc->root;
    while (root.size() > 1 && root[root.size()-1] == '/')
        root = root.substr(0, root.size()-1);
    std::string rel = req.getPath();
    if (rel.empty() || rel[0] != '/') rel = "/" + rel;
    std::string fullPath = root + rel;

    if (isCgiPath(*loc, req.getPath())) {
        CgiHandler cgi;
        cgi.handle(*srv, *loc, req, resp, fullPath);
    } else if (method == "POST") {
        if (!loc->uploadPath.empty()) {
            UploadHandler upload;
            upload.handle(*srv, *loc, req, resp);
        } else {
            // Try CGI or 405
            resp.setStatus(405);
            resp.setHeader("Allow", "GET, DELETE");
            resp.setBody(HttpResponse::buildError(405), "text/html");
        }
    } else {
        StaticHandler staticH;
        staticH.handle(*srv, *loc, req, resp);
    }

    client.setState(CS_SENDING);
}

void ServerManager::removeClient(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;
    delete it->second;
    _clients.erase(it);
}
