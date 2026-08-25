#include "Connection.hpp"
#include "Router.hpp"
#include "HandlerFactory.hpp"
#include "ErrorResponse.hpp"
#include "ResponseSerializer.hpp"
#include "CgiProcess.hpp"
#include "Exceptions.hpp"
#include "Time.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int clientFd,
                       const std::string& clientIp,
                       int clientPort,
                       const std::vector<ServerConfig>& servers,
                       int serverPort,
                       PollRegistry& registry)
    : _socket(clientFd),
      _state(CONN_STATE_READING),
      _lastActivity(Time::now()),
      _timeoutSeconds(60),
      _servers(servers),
      _serverPort(serverPort),
      _clientIp(clientIp),
      _clientPort(clientPort),
      _cgiProcess(NULL),
      _registry(&registry),
      _keepAlive(true) {
    Socket::setNonBlocking(clientFd);
}

Connection::~Connection() {
    close();
}

int Connection::getFd() const {
    return _socket.getFd();
}

std::time_t Connection::getLastActivity() const {
    return _lastActivity;
}

bool Connection::isTimedOut(std::time_t now) const {
    return (now - _lastActivity) > _timeoutSeconds;
}

void Connection::close() {
    if (_cgiProcess) {
        delete _cgiProcess;
        _cgiProcess = NULL;
    }
    _socket.close();
    _state = CONN_STATE_CLOSED;
}

void Connection::handleTimeout() {
    Logger::info("Connection timed out on fd " + StringUtils::toString(getFd()));
    if (_state == CONN_STATE_READING && _inBuffer.size() > 0) {
        sendError(408);
    } else {
        close();
    }
}

void Connection::sendError(int statusCode, const ServerConfig* server) {
    HttpResponse errResp = ErrorResponse::build(statusCode, server);
    errResp.setKeepAlive(false);
    _keepAlive = false;
    std::string serialized = ResponseSerializer::serialize(errResp);
    _outBuffer.clear();
    _outBuffer.append(serialized);
    _state = CONN_STATE_WRITING;
}

void Connection::processRequest() {
    HttpRequest& req = _parser.getRequest();
    req.setClientIp(_clientIp);
    req.setClientPort(_clientPort);

    _keepAlive = req.isKeepAlive();

    try {
        RequestContext ctx = Router::route(_servers, req, _serverPort);

        if (ctx.isCgi()) {
            _cgiProcess = new CgiProcess(ctx);
            if (!_cgiProcess->launch(*_registry)) {
                if (_cgiProcess->isError()) {
                    sendError(_cgiProcess->getErrorCode(), ctx.getServer());
                } else {
                    sendError(500, ctx.getServer());
                }
                delete _cgiProcess;
                _cgiProcess = NULL;
                return;
            }
            _state = CONN_STATE_WAIT_CGI;
            return;
        }

        IRequestHandler* handler = HandlerFactory::createHandler(ctx);
        HttpResponse response = handler->handle(ctx);
        delete handler;

        response.setKeepAlive(_keepAlive);
        std::string serialized = ResponseSerializer::serialize(response);
        _outBuffer.append(serialized);
        _state = CONN_STATE_WRITING;

    } catch (const HttpError& e) {
        const ServerConfig& srv = Router::matchServer(_servers, req.getHost(), _serverPort);
        sendError(e.getStatusCode(), &srv);
    } catch (...) {
        sendError(500, NULL);
    }
}

void Connection::handleRead() {
    if (_state != CONN_STATE_READING) {
        return;
    }

    char buffer[65536];
    ssize_t bytes = recv(_socket.getFd(), buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        // EOF or client disconnected
        close();
        return;
    }

    _lastActivity = Time::now();

    if (!_parser.feed(buffer, bytes)) {
        if (_parser.isError()) {
            sendError(_parser.getErrorCode());
        } else {
            sendError(400);
        }
        return;
    }

    if (_parser.isComplete()) {
        _state = CONN_STATE_PROCESSING;
        processRequest();
    }
}

void Connection::handleWrite() {
    _lastActivity = Time::now();

    if (_state == CONN_STATE_WAIT_CGI) {
        if (_cgiProcess) {
            _cgiProcess->checkTimeout();
            if (_cgiProcess->isDone()) {
                HttpResponse response;
                if (_cgiProcess->isError()) {
                    response = ErrorResponse::build(_cgiProcess->getErrorCode());
                } else {
                    response = _cgiProcess->getResponse();
                }
                delete _cgiProcess;
                _cgiProcess = NULL;

                response.setKeepAlive(_keepAlive);
                std::string serialized = ResponseSerializer::serialize(response);
                _outBuffer.append(serialized);
                _state = CONN_STATE_WRITING;
            }
        }
    }

    if (_state == CONN_STATE_WRITING) {
        if (_outBuffer.empty()) {
            if (_keepAlive) {
                _parser.reset();
                _inBuffer.clear();
                _state = CONN_STATE_READING;
            } else {
                close();
            }
            return;
        }

        ssize_t bytes = send(_socket.getFd(), _outBuffer.data(), _outBuffer.size(), 0);
        if (bytes > 0) {
            _outBuffer.consume(bytes);
        } else if (bytes < 0) {
            close();
            return;
        }

        if (_outBuffer.empty()) {
            if (_keepAlive) {
                _parser.reset();
                _inBuffer.clear();
                _state = CONN_STATE_READING;
            } else {
                close();
            }
        }
    }
}

bool Connection::wantsRead() const {
    return _state == CONN_STATE_READING && !isDead();
}

bool Connection::wantsWrite() const {
    return (_state == CONN_STATE_WRITING || _state == CONN_STATE_WAIT_CGI) && !isDead();
}

bool Connection::isDead() const {
    return _state == CONN_STATE_CLOSED || _socket.getFd() < 0;
}
