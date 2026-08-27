*This project has been created as part of the 42 curriculum by mjehad.*

# webserv — HTTP/1.1 Web Server

An HTTP/1.1 web server implemented in C++ 98 using non-blocking I/O multiplexing with a single `poll()` event loop. Designed to be resilient, performant, and compliant with the HTTP/1.1 RFC specifications (RFC 7230, RFC 7231) and the Common Gateway Interface specification (RFC 3875).

---

## Description

**webserv** is an HTTP/1.1 web server built from scratch without external dependencies or third-party libraries. The server handles concurrent client connections using event-driven reactor pattern driven by `poll()`, processing requests non-blockingly across listening sockets, client TCP streams, and asynchronous CGI communication pipes.

### Key Capabilities

- **I/O Multiplexing**: Single-threaded, non-blocking event loop using `poll()` to monitor read and write readiness simultaneously across all active sockets and CGI pipes.
- **HTTP/1.1 Support**:
  - Methods: `GET`, `POST`, `DELETE`, `HEAD`.
  - Chunked transfer request body decoding (`Transfer-Encoding: chunked`).
  - Persistent connections with `Connection: keep-alive` and `close`.
  - Client request body limits (`client_max_body_size`) returning `413 Payload Too Large`.
  - Custom and built-in error page templates (400, 403, 404, 405, 408, 413, 414, 431, 500, 502, 504).
  - Bounded request line, header section and body sizes, with a lingering close so
    a rejected upload can still read its `413` response.
  - Separate idle keep-alive and in-flight request timeouts, so no request hangs indefinitely.
- **Static File Serving & Routing**:
  - MIME type detection for standard web assets (HTML, CSS, JS, images, audio, video).
  - Configurable root directories and default index files (`index.html`).
  - Autoindex directory listing generation for folders lacking index files.
  - HTTP Redirection (`return 301` / `return 302`).
  - File uploads via `POST` with `upload_dir` configuration.
  - File deletion via `DELETE`.
- **CGI Execution (RFC 3875)**:
  - Non-blocking pipe-based execution of scripts (.py, .sh, .php, etc.).
  - Environment variable setup (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `PATH_INFO`, `SCRIPT_NAME`, `HTTP_*`, `REDIRECT_STATUS`, etc.).
  - Relative execution directory (`chdir`).
  - Chunked request bodies are un-chunked so the script sees `EOF` at the end of the body.
  - Process timeout protection (`504 Gateway Timeout`), guaranteed child reaping
    (no zombies), and `FD_CLOEXEC` so children never inherit server sockets.
- **Bonus Features**:
  - Session and cookie management with `Set-Cookie` and `Cookie` headers.
  - Multi-port and virtual host configuration matching `server_name` and `Host` headers.
  - Support for multiple CGI scripting languages.

---

## Instructions

### 1. Requirements & Compatibility
- Linux or macOS operating system
- `c++` compiler (`clang++` or `g++`) supporting `-std=c++98`
- `make` build utility
- Python 3 (for running automated test suite and Python CGI scripts)

### 2. Compilation
Compile the server using the provided Makefile:

```bash
make
```

Makefile targets:
- `make` or `make all`: Compiles the `webserv` executable with `-Wall -Wextra -Werror -std=c++98`.
- `make clean`: Removes intermediate object files (`.o`).
- `make fclean`: Removes object files and the `webserv` binary.
- `make re`: Performs a full recompilation (`fclean` + `all`).

### 3. Execution
Run `webserv` with a path to a configuration file, or without arguments to use the default configuration:

```bash
# Using default configuration (config/default.conf)
./webserv

# Using a custom configuration file
./webserv config/default.conf
./webserv config/multi_port.conf
./webserv config/cgi.conf
```

### 4. Running the Automated Test Suite
Two suites are provided. Start the server first, then run them against it:

```bash
# 1. Start webserv in background
./webserv config/default.conf &
SERVER_PID=$!

# 2. Functional suite: methods, routing, CGI, uploads, status codes
python3 tests/integration/test_webserv.py

# 3. Regression & robustness suite: RFC edge cases, malformed input,
#    client disconnects, process/fd hygiene and stress tests
python3 tests/integration/test_regression.py

#    Add --slow to also exercise the request-timeout paths (~25s extra)
python3 tests/integration/test_regression.py --slow

# 4. Terminate server
kill $SERVER_PID
```

To check for memory and descriptor leaks:

```bash
valgrind --leak-check=full --track-fds=yes ./webserv config/default.conf
# run the suites above, then stop the server with Ctrl-C
```

---

## Configuration Grammar

Configuration files use an NGINX-inspired block syntax:

```nginx
server {
    listen        127.0.0.1:8080;
    server_name   localhost;
    client_max_body_size 10M;

    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    root www/site;
    index index.html;

    location / {
        root www/site;
        index index.html;
        methods GET POST DELETE;
        autoindex on;
    }

    location /uploads {
        root www/uploads;
        methods GET POST DELETE;
        upload_dir www/uploads;
        autoindex on;
    }

    location /cgi-bin {
        root www/cgi-bin;
        methods GET POST;
        cgi_ext .py /usr/bin/python3;
        cgi_ext .sh /bin/bash;
    }

    location /redirect {
        return 301 /;
    }
}
```

---

## Resources

### References & Documentation
- **RFC 7230**: *Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing*
- **RFC 7231**: *Hypertext Transfer Protocol (HTTP/1.1): Semantics and Content*
- **RFC 3875**: *The Common Gateway Interface (CGI) Version 1.1*
- **Beej's Guide to Network Programming**: Sockets, non-blocking I/O, `poll()`, and `fcntl()`.
- **NGINX Documentation**: Configuration structure and directive semantics.

### AI Usage Disclosure
Artificial Intelligence (LLM assistance) was utilized during the development of this project for:
1. **Architecture & Modular Decomposition**: Planning strict separation of concerns into isolated modules (`core`, `config`, `net`, `connection`, `http`, `routing`, `handlers`, `cgi`, `util`) to meet file length (< 250 lines) and function length (< 75 lines) constraints.
2. **Grammar & Parser Design**: Formulating the recursive-descent configuration tokenizer and parser for NGINX-like config files.
3. **CGI Non-Blocking Pipeline**: Designing the asynchronous pipe event handler architecture to drive child stdin/stdout multiplexing entirely through the single `poll()` reactor without blocking the main event loop.
4. **Automated Test Generation**: Creating the comprehensive Python integration test suite covering RFC status codes, chunked encoding, multipart uploads, cookie sessions, and edge-case handling.
