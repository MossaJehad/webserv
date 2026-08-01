#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <string>
#include <vector>
#include "Config.hpp"

// Everything the poll loop needs to know about one accepted client fd.
// No HTTP parsing here on purpose - just raw byte buffers - that is the
// next layer to be built on top of this one.
class ClientConnection {
public:
    enum State {
        READING_REQUEST,   // waiting for / accumulating request bytes
        SENDING_RESPONSE,  // response ready, draining write buffer
        CLOSING            // about to be removed from poll set
    };

private:
    int         _fd;
    int         _listen_fd;                       // which listening socket accepted us
    std::vector<const ServerConfig*> _candidates;  // server{} blocks sharing that listen_fd

    std::string _read_buffer;
    std::string _write_buffer;
    size_t      _write_offset;

    State       _state;
    bool        _should_close_after_write;

public:
    ClientConnection();
    ClientConnection(int fd, int listen_fd, const std::vector<const ServerConfig*>& candidates);
    ~ClientConnection();

    int    get_fd() const;
    State  get_state() const;

    // Called only after poll() reported POLLIN on this fd.
    // Returns: 1 = peer closed / fatal error (caller must close fd),
    //          0 = would block or read fine, keep going.
    int  on_readable();

    // Called only after poll() reported POLLOUT on this fd.
    // Returns: 1 = peer closed / fatal error, 0 = ok.
    int  on_writable();

    bool wants_to_read() const;   // true -> register POLLIN
    bool wants_to_write() const;  // true -> register POLLOUT
    bool should_close() const;

private:
    bool headers_complete() const;
    void build_stub_response();
};

#endif
