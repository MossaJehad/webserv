#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "IEventHandler.hpp"
#include "Socket.hpp"
#include "IoBuffer.hpp"
#include "RequestParser.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "PollRegistry.hpp"
#include <vector>
#include <ctime>

class CgiProcess;

enum ConnectionState {
    CONN_STATE_READING,
    CONN_STATE_PROCESSING,
    CONN_STATE_WAIT_CGI,
    CONN_STATE_WRITING,
    CONN_STATE_LINGER,
    CONN_STATE_CLOSED
};

class Connection : public IEventHandler {
private:
    Socket _socket;
    IoBuffer _inBuffer;
    IoBuffer _outBuffer;
    RequestParser _parser;
    ConnectionState _state;
    std::time_t _lastActivity;
    int _idleTimeoutSeconds;    // idle keep-alive socket
    int _requestTimeoutSeconds; // half-received request in flight
    int _lingerTimeoutSeconds;  // draining a rejected upload before closing

    std::vector<ServerConfig> _servers;
    int _serverPort;
    std::string _clientIp;
    int _clientPort;

    CgiProcess* _cgiProcess;
    PollRegistry* _registry;
    bool _keepAlive;
    bool _lingerOnClose;

    void processRequest();
    void sendError(int statusCode, const ServerConfig* server = NULL);
    void applyHeadSemantics(HttpResponse& response) const;
    void queueResponse(HttpResponse& response);
    void consume(const char* data, size_t len);
    void drainWhileBusy();
    void finishResponse();
    void drainLinger();

public:
    Connection(int clientFd,
               const std::string& clientIp,
               int clientPort,
               const std::vector<ServerConfig>& servers,
               int serverPort,
               PollRegistry& registry);
    virtual ~Connection();

    virtual int getFd() const;
    virtual void handleRead();
    virtual void handleWrite();
    virtual bool wantsRead() const;
    virtual bool wantsWrite() const;
    virtual bool isDead() const;
    virtual void handleTimeout();

    void checkCgi();
    ConnectionState getState() const;

    std::time_t getLastActivity() const;
    bool isTimedOut(std::time_t now) const;
    void close();
};

#endif
