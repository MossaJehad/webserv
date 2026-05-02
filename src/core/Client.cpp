#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <iostream>

Client::Client(int fd, const std::string& ip, const ServerConfig* srv)
    : _fd(fd), _ip(ip), _state(CS_READING), _srv(srv),
      _sendOffset(0), _lastActivity(time(NULL))
{}

Client::~Client()
{
    if (_fd >= 0)
        close(_fd);
}

int         Client::getFd()            const { return _fd; }
ClientState Client::getState()         const { return _state; }
void        Client::setState(ClientState s)   { _state = s; }
HttpRequest&  Client::getRequest()           { return _request; }
HttpResponse& Client::getResponse()          { return _response; }
const ServerConfig* Client::getServerConfig() const { return _srv; }
void Client::setServerConfig(const ServerConfig* srv) { _srv = srv; }
time_t Client::getLastActivity() const { return _lastActivity; }
void   Client::updateActivity()        { _lastActivity = time(NULL); }
const std::string& Client::getIp() const { return _ip; }

bool Client::isKeepAlive() const
{
    return _request.isComplete() && _request.isKeepAlive();
}

void Client::reset()
{
    _request.reset();
    _response   = HttpResponse();
    _sendBuf.clear();
    _sendOffset = 0;
    _state      = CS_READING;
    updateActivity();
}

bool Client::readData()
{
    char buf[8192];
    ssize_t n = recv(_fd, buf, sizeof(buf), 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true; // no data yet, try later
        return false; // real error
    }
    if (n == 0)
        return false; // connection closed

    updateActivity();
    _request.feed(buf, static_cast<size_t>(n));
    return true;
}

bool Client::writeData()
{
    if (_sendBuf.empty()) {
        _sendBuf   = _response.build();
        _sendOffset = 0;
    }

    while (_sendOffset < _sendBuf.size()) {
        ssize_t n = send(_fd, _sendBuf.c_str() + _sendOffset,
                         _sendBuf.size() - _sendOffset, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true; // partial — POLLOUT will fire again
            return false;    // real error
        }
        if (n == 0) return false;
        _sendOffset += static_cast<size_t>(n);
        updateActivity();
    }

    // All bytes sent — leave _sendBuf intact so isSendDone() returns true
    return true;
}

bool Client::isSendDone() const
{
    return !_sendBuf.empty() && _sendOffset >= _sendBuf.size();
}
