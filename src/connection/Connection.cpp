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

namespace {

// Largest body any server on this listener would accept. Used as a parse-time
// guard so an oversized upload is rejected before it is buffered in memory;
// the exact per-location limit is still enforced by the Router.
size_t maxAcceptedBodySize(const std::vector<ServerConfig>& servers) {
    size_t limit = 0;
    for (size_t i = 0; i < servers.size(); ++i) {
        if (servers[i].getClientMaxBodySize() > limit) {
            limit = servers[i].getClientMaxBodySize();
        }
        const std::vector<LocationConfig>& locations = servers[i].getLocations();
        for (size_t l = 0; l < locations.size(); ++l) {
            if (locations[l].hasBodySizeLimit() && locations[l].getClientMaxBodySize() > limit) {
                limit = locations[l].getClientMaxBodySize();
            }
        }
    }
    return limit;
}

} // namespace

Connection::Connection(int clientFd,
                       const std::string& clientIp,
                       int clientPort,
                       const std::vector<ServerConfig>& servers,
                       int serverPort,
                       PollRegistry& registry)
    : _socket(clientFd),
      _state(CONN_STATE_READING),
      _lastActivity(Time::now()),
      _idleTimeoutSeconds(60),
      _requestTimeoutSeconds(20),
      _lingerTimeoutSeconds(5),
      _servers(servers),
      _serverPort(serverPort),
      _clientIp(clientIp),
      _clientPort(clientPort),
      _cgiProcess(NULL),
      _registry(&registry),
      _keepAlive(true),
      _lingerOnClose(false) {
    Socket::setNonBlocking(clientFd);
    // A CGI child must not inherit this socket, otherwise the peer would not
    // see the connection close until that unrelated child exits.
    Socket::setCloexec(clientFd);
    _parser.setMaxBodySize(maxAcceptedBodySize(servers));
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
    // A peer that started a request but stalled mid-message is cut off sooner
    // than an idle keep-alive socket, so no request can hang indefinitely.
    int limit = _idleTimeoutSeconds;
    if (_state == CONN_STATE_LINGER) {
        limit = _lingerTimeoutSeconds;
    } else if (_state == CONN_STATE_READING && _parser.hasPartialRequest()) {
        limit = _requestTimeoutSeconds;
    }
    return (now - _lastActivity) > limit;
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

    // A half-sent request earns a 408; an idle keep-alive socket is simply
    // dropped, which is what NGINX does.
    if (_state == CONN_STATE_READING && _parser.hasPartialRequest()) {
        sendError(408);
    } else {
        close();
    }
}

void Connection::sendError(int statusCode, const ServerConfig* server) {
    HttpResponse errResp = ErrorResponse::build(statusCode, server);
    errResp.setKeepAlive(false);
    _keepAlive = false;
    // Answering before the announced body arrived means the peer is still
    // sending; remember to drain instead of resetting the connection.
    _lingerOnClose = _parser.wasBodyTruncated();
    std::string serialized = ResponseSerializer::serialize(errResp);
    _outBuffer.clear();
    _outBuffer.append(serialized);
    _state = CONN_STATE_WRITING;
    _lastActivity = Time::now(); // give the error a fresh window to be flushed
}

// RFC 7231 4.3.2: a HEAD response must carry the same header fields as the
// equivalent GET, but no payload. setBody() recomputes Content-Length, so the
// real length is restored afterwards.
void Connection::applyHeadSemantics(HttpResponse& response) const {
    if (_parser.getRequest().getMethod() != METHOD_HEAD || response.isRaw()) {
        return;
    }
    size_t realLength = response.getBody().size();
    if (response.getHeaders().has("Content-Length")) {
        realLength = StringUtils::toSizeT(response.getHeaders().get("Content-Length"));
    }
    response.setBody("");
    response.getHeaders().set("Content-Length", StringUtils::toString(realLength));
}

void Connection::queueResponse(HttpResponse& response) {
    applyHeadSemantics(response);
    response.setKeepAlive(_keepAlive);
    _outBuffer.append(ResponseSerializer::serialize(response));
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
                int code = _cgiProcess->isError() ? _cgiProcess->getErrorCode() : 500;
                delete _cgiProcess;
                _cgiProcess = NULL;
                sendError(code, ctx.getServer());
                return;
            }
            _state = CONN_STATE_WAIT_CGI;
            return;
        }

        IRequestHandler* handler = HandlerFactory::createHandler(ctx);
        HttpResponse response = handler->handle(ctx);
        delete handler;

        queueResponse(response);

    } catch (const HttpError& e) {
        const ServerConfig& srv = Router::matchServer(_servers, req.getHost(), _serverPort);
        sendError(e.getStatusCode(), &srv);
    } catch (...) {
        sendError(500, NULL);
    }
}

void Connection::handleRead() {
    if (_state == CONN_STATE_LINGER) {
        drainLinger();
        return;
    }

    if (_state == CONN_STATE_WAIT_CGI) {
        drainWhileBusy();
        return;
    }

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
    consume(buffer, static_cast<size_t>(bytes));
}

void Connection::consume(const char* data, size_t len) {
    if (!_parser.feed(data, len)) {
        sendError(_parser.isError() ? _parser.getErrorCode() : 400);
        return;
    }

    if (_parser.isComplete()) {
        // Bytes beyond this message belong to the next pipelined request; keep
        // them, because resetting the parser would otherwise discard them.
        std::string leftover = _parser.takeLeftover();
        if (!leftover.empty()) {
            _inBuffer.append(leftover);
        }
        _state = CONN_STATE_PROCESSING;
        processRequest();
    }
}

// While a CGI child owns the response, the client socket stays in the poll set
// so a disconnect is noticed immediately instead of waiting for the idle
// timeout. Anything the client pipelines is stashed and replayed later.
void Connection::drainWhileBusy() {
    char buffer[8192];
    ssize_t bytes = recv(_socket.getFd(), buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        Logger::info("Client disconnected while CGI was running on fd " +
                     StringUtils::toString(getFd()));
        close(); // also kills and reaps the CGI child
        return;
    }

    _lastActivity = Time::now();
    _inBuffer.append(buffer, static_cast<size_t>(bytes));
}

void Connection::checkCgi() {
    if (_state != CONN_STATE_WAIT_CGI || !_cgiProcess) {
        return;
    }

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

        queueResponse(response);
    }
}

ConnectionState Connection::getState() const {
    return _state;
}

void Connection::finishResponse() {
    if (!_keepAlive) {
        if (_lingerOnClose) {
            // The peer is probably still pushing a body we refused. Closing now
            // would reset the connection and destroy the response it has not
            // read yet, so drain quietly until it stops or the linger expires.
            _state = CONN_STATE_LINGER;
            _lastActivity = Time::now();
            return;
        }
        close();
        return;
    }

    _parser.reset();
    _state = CONN_STATE_READING;

    // Replay bytes that arrived while the previous response was in flight.
    if (!_inBuffer.empty()) {
        std::string pending = _inBuffer.str();
        _inBuffer.clear();
        consume(pending.data(), pending.size());
    }
}

void Connection::drainLinger() {
    char scratch[65536];
    ssize_t bytes = recv(_socket.getFd(), scratch, sizeof(scratch), 0);
    if (bytes <= 0) {
        close(); // peer finished or vanished
    }
    // Data is deliberately discarded: a response has already been sent.
}

void Connection::handleWrite() {
    if (_state != CONN_STATE_WRITING) {
        return;
    }

    _lastActivity = Time::now();

    if (_outBuffer.empty()) {
        finishResponse();
        return;
    }

    ssize_t bytes = send(_socket.getFd(), _outBuffer.data(), _outBuffer.size(), 0);
    if (bytes > 0) {
        _outBuffer.consume(static_cast<size_t>(bytes));
    } else if (bytes < 0) {
        close();
        return;
    }

    if (_outBuffer.empty()) {
        finishResponse();
    }
}

bool Connection::wantsRead() const {
    if (isDead()) {
        return false;
    }
    return _state == CONN_STATE_READING ||
           _state == CONN_STATE_WAIT_CGI ||
           _state == CONN_STATE_LINGER;
}

bool Connection::wantsWrite() const {
    return _state == CONN_STATE_WRITING && !isDead();
}

bool Connection::isDead() const {
    return _state == CONN_STATE_CLOSED || _socket.getFd() < 0;
}
