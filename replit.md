# webserv — C++98 HTTP Server

## Overview
A fully non-blocking HTTP/1.1 server written in C++98 for the 42 School "webserv" project.

## Build
```bash
make        # build
make re     # clean rebuild
make clean  # remove objects
make fclean # remove objects + binary
```

Compiler flags: `-std=c++98 -Wall -Wextra -Werror`

## Run
```bash
./webserv webserv.conf       # default config
./webserv path/to/my.conf    # custom config
```

## Architecture

```
src/
├── main.cpp
├── config/
│   ├── ConfigParser.hpp/cpp   — nginx-style .conf parser
│   ├── ServerConfig.hpp/cpp   — per-server block (host, port, error_pages, locations)
│   └── LocationConfig.hpp/cpp — per-route config (root, methods, CGI, upload, etc.)
├── core/
│   ├── ServerManager.hpp/cpp  — single poll() loop, accept/dispatch
│   └── Client.hpp/cpp         — per-connection state machine (READING→PROCESSING→SENDING)
├── http/
│   ├── HttpRequest.hpp/cpp    — request line + header + body parser (chunked TE)
│   ├── HttpResponse.hpp/cpp   — response builder (status, headers, body, MIME)
│   └── Router.hpp/cpp         — URI→LocationConfig matching, method/size checks
└── handlers/
    ├── StaticHandler.hpp/cpp  — GET (files + autoindex), DELETE
    ├── UploadHandler.hpp/cpp  — POST multipart/form-data → disk
    └── CgiHandler.hpp/cpp     — fork/execve, pipe I/O via poll, CGI/1.1 env
```

## Configuration (nginx-like)

```nginx
server {
    listen 5000;
    server_name localhost;
    client_max_body_size 10m;

    error_page 404 ./www/html/errors/404.html;

    location / {
        root ./www/html;
        index index.html;
        allow_methods GET POST DELETE;
        autoindex off;
    }

    location /files/ {
        root ./www/html;
        allow_methods GET DELETE;
        autoindex on;
    }

    location /upload/ {
        root ./www/html;
        allow_methods GET POST;
        upload_path ./www/uploads;
        client_max_body_size 20m;
    }

    location /cgi-bin/ {
        root ./www;
        allow_methods GET POST;
        cgi_ext .py .php .sh;
    }

    location /old/ {
        return 301 /new/;
    }
}
```

## Key Design Decisions
- **Single `poll()` loop** in `ServerManager` — all fds (listen + clients) multiplexed
- **Non-blocking everywhere** — `O_NONBLOCK` via `fcntl` on all sockets
- **`nginx root` semantics** — `root + full_uri_path` (no location prefix stripping)
- **CGI** uses `fork()` + `execve()` + `pipe()`, output read via `poll()` with 10s timeout
- **Virtual hosts** matched by `Host` header against `server_name` directives
- **Keep-alive** — client resets to `CS_READING` state after response if `Connection: keep-alive`
- **Body size** check per-location (`client_max_body_size`)
- **Custom error pages** per server block, fallback to inline HTML

## Tested Features
| Feature | Status |
|---|---|
| GET static files | ✅ |
| Directory index | ✅ |
| Autoindex listing | ✅ |
| DELETE files | ✅ |
| POST multipart upload | ✅ |
| CGI Python (GET + query string) | ✅ |
| CGI Python (POST + body) | ✅ |
| CGI Shell scripts | ✅ |
| 404 / 403 / 405 / 413 error pages | ✅ |
| 301/302 redirects | ✅ |
| Virtual hosts / multiple ports | ✅ |
| HTTP/1.1 keep-alive | ✅ |
| Chunked transfer encoding | ✅ |
| Multiple simultaneous clients | ✅ |

## File Layout
```
webserv/
├── Makefile
├── webserv.conf
├── src/           ← C++ source
├── www/
│   ├── html/      ← static files
│   │   ├── index.html
│   │   ├── files/    ← autoindex demo
│   │   └── errors/   ← custom error pages
│   ├── cgi-bin/   ← CGI scripts (hello.py, info.sh)
│   └── uploads/   ← upload destination
└── obj/           ← compiled objects (gitignored)
```

## Dependencies
- C++98 compiler (g++/clang++)
- Python 3 (for CGI scripts) — installed as Replit module
- POSIX sockets, poll(), fork(), execve() — all system calls
