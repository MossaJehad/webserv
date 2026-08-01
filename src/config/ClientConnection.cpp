#include "ClientConnection.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

static const size_t READ_CHUNK = 4096;

ClientConnection::ClientConnection()
    : _fd(-1), _listen_fd(-1), _write_offset(0),
      _state(READING_REQUEST), _should_close_after_write(false)
{
}

ClientConnection::ClientConnection(int fd, int listen_fd, const std::vector<const ServerConfig*>& candidates)
    : _fd(fd), _listen_fd(listen_fd), _candidates(candidates), _write_offset(0),
      _state(READING_REQUEST), _should_close_after_write(false)
{
}

ClientConnection::~ClientConnection()
{
    // fd lifetime (close()) is managed by WebServer, not here, so that
    // copies stored in containers never accidentally close a live fd.
}

int ClientConnection::get_fd() const
{
    return _fd;
}

ClientConnection::State ClientConnection::get_state() const
{
    return _state;
}

bool ClientConnection::wants_to_read() const
{
    return _state == READING_REQUEST;
}

bool ClientConnection::wants_to_write() const
{
    return _state == SENDING_RESPONSE && _write_offset < _write_buffer.size();
}

bool ClientConnection::should_close() const
{
    return _state == CLOSING;
}

bool ClientConnection::headers_complete() const
{
    return _read_buffer.find("\r\n\r\n") != std::string::npos;
}

// Placeholder response so the socket/poll plumbing is demonstrable with
// curl/telnet/a browser before the real HTTP parser + router exist.
void ClientConnection::build_stub_response()
{
    std::string body = "webserv: socket layer alive, ";
    body += "listen_fd=";
    std::ostringstream oss;
    oss << _listen_fd;
    body += oss.str();
    body += ", " ;
    oss.str("");
    oss << _candidates.size();
    body += oss.str();
    body += " server block(s) reachable on this listener.\n";

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: text/plain\r\n"
              << "Content-Length: " << body.size() << "\r\n"
              << "Connection: close\r\n"
              << "\r\n"
              << body;

    _write_buffer = response.str();
    _write_offset = 0;
    _state = SENDING_RESPONSE;
}

int ClientConnection::on_readable()
{
    char buf[READ_CHUNK];
    ssize_t n = recv(_fd, buf, READ_CHUNK, 0);

    if (n == 0)   // orderly shutdown by peer
    {
        _state = CLOSING;
        return 1;
    }
    if (n < 0)    // real error on an fd poll marked readable
    {
        _state = CLOSING;
        return 1;
    }

    _read_buffer.append(buf, static_cast<size_t>(n));

    if (headers_complete())
        build_stub_response();

    return 0;
}

int ClientConnection::on_writable()
{
    if (_write_offset >= _write_buffer.size())
        return 0;

    const char* data = _write_buffer.c_str() + _write_offset;
    size_t      remaining = _write_buffer.size() - _write_offset;

    ssize_t n = send(_fd, data, remaining, 0);

    if (n <= 0)
    {
        _state = CLOSING;
        return 1;
    }

    _write_offset += static_cast<size_t>(n);

    if (_write_offset >= _write_buffer.size())
        _state = CLOSING; // response fully flushed, connection: close was advertised
    return 0;
}
