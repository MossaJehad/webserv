# Webserv — Architecture Index

C++98 HTTP/1.1 server. Single process. One non-blocking `poll()` loop.

Full architecture split across files:

- High-Level Architecture → [MDFiles/01_High_Level_Architecture.md](../MDFiles/01_High_Level_Architecture.md)
- Module Responsibilities → [MDFiles/04_Module_Responsibilities.md](../MDFiles/04_Module_Responsibilities.md)
- Data Flow → [MDFiles/06_Data_Flow.md](../MDFiles/06_Data_Flow.md)
- Shared contracts → [docs/CONTRACTS.md](CONTRACTS.md)
- Config grammar → [docs/CONFIG_GRAMMAR.md](CONFIG_GRAMMAR.md)

## Rules

- C++98 only. Flags: `-Wall -Wextra -Werror -std=c++98`.
- One `poll()` for all fds: listeners, clients, CGI pipes.
- Read AND write checked same cycle. No `errno` after read/write.
- Every fd has one `IEventHandler`. Loop dumb, handlers smart.
- One class = one file pair. File < 250 lines. Function < 75 lines.
- Small focused classes. No God Object. Composition over inheritance.

## Layers

```
main -> core(boot) -> config(load) -> net(listen) -> Reactor(poll)
                                                          |
                              connection(state machine) + cgi pipes
                                          |
              request bytes -> http parse -> routing -> handlers -> response -> write
```

## Dependency direction

`util <- config <- routing/handlers`. `net <- connection <- http`.
`core` wires all. Lower layer never calls upper layer.
