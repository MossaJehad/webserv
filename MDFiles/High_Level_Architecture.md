# 1. High-Level Architecture

Single process. Single non-blocking event loop (`poll`). No threads.
No fork except CGI.

## Layers (bottom to top)

```
main -> Core(boot) -> Config(load) -> Net(listen) -> Reactor(poll loop)
                                                          |
                              +---------------------------+--------------------------+
                              v                                                      v
                        Connection (state machine)                              CGI pipes
                              |
              Request bytes -> HTTP parse -> Router -> Handler -> Response -> write
```

Config flows down once at boot. Request flows up per request.
Response flows back down.

## Rules baked in

- One `poll()` watches all fds: listeners, clients, CGI pipes.
- Read AND write checked in same poll cycle.
- No `errno` check after `read`/`write`. React only to poll flags.
- Every fd has one `IEventHandler`. Loop dumb. Handlers smart.
- Each class = one job = one file pair. Files stay small.

## Constraints

- C++98. Flags `-Wall -Wextra -Werror -std=c++98`.
- File < 250 lines. Function < 75 lines.
- Small focused classes. No God Object. No monolith file.
- Composition over big inheritance trees.

## Why this shape

- Single loop = required by subject (one poll for all I/O).
- Layered = low coupling, parallel teamwork, easy test.
- Data-only request/response = unit-test without sockets.
- Interfaces (IEventHandler, IRequestHandler) = extend without editing core.
