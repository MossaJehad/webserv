# 4. Module Responsibilities

## Module table

| Module | Owns | Does NOT do |
|---|---|---|
| **core** | boot order, lifetime, signals | parse, I/O |
| **config** | read .conf, build config model, validate | network, http |
| **net** | sockets, listen, accept, poll loop, dispatch | http meaning |
| **connection** | client state machine, buffers, timeout | parse rules, routing |
| **http** | parse request, build/serialize response | sockets, files, config match |
| **routing** | match vhost + location, resolve path, method allow | I/O, response body |
| **handlers** | produce response per method/route | sockets, parsing wire |
| **cgi** | run script, pipe I/O, parse cgi output | http wire parse |
| **util** | generic helpers | domain logic |

## Dependency direction

- `util <- config <- routing/handlers`
- `net <- connection <- http`
- `core` wires all. Lower never calls upper.

## Owner map

| Module | Owner |
|---|---|
| core | Developer A |
| config | Developer A |
| util | Developer A |
| net | Developer B |
| connection | Developer B |
| http | Developer C |
| routing | Developer C |
| handlers | Developer C |
| cgi | Developer C |

## Class responsibilities (summary)

- `Webserv` — load config, build listeners, register, run. Thin.
- `Signal` — install handlers, set stop flag.
- `ServerConfig` / `LocationConfig` — config data holders.
- `ConfigTokenizer` / `ConfigParser` / `ConfigValidator` — read, parse, check.
- `Socket` — RAII fd + non-block.
- `Listener` — bind/listen/accept; IEventHandler.
- `PollRegistry` — fd<->handler map, build pollfd[].
- `Reactor` — the poll() loop, dispatch.
- `IoBuffer` — in/out byte buffer.
- `Connection` — per-client state machine; IEventHandler. Fattest, watch it.
- `ConnectionManager` — create/destroy, idle timeout.
- `HttpRequest` / `HttpResponse` — data holders.
- `HttpHeaders` — case-insensitive store.
- `RequestParser` + `ChunkedDecoder` — incremental parse.
- `HttpStatus` — code -> reason.
- `ResponseSerializer` — response -> bytes.
- `RequestContext` — request + config + path bundle.
- `Router` — vhost + location match, path resolve.
- `IRequestHandler` / `HandlerFactory` — interface + dispatch.
- `StaticFileHandler` / `AutoindexHandler` / `UploadHandler` /
  `DeleteHandler` / `RedirectHandler` — one job each.
- `ErrorResponse` — central error page builder.
- `CgiEnvironment` / `CgiProcess` / `CgiResponseParser` — CGI run + parse.
- `util/*` — string, fs, mime, log, time, exceptions. Stateless helpers.
