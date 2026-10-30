# 6. Data Flow

## Boot (once)

```
.conf file -> ConfigTokenizer -> ConfigParser -> ConfigValidator
           -> vector<ServerConfig>
           -> Listener per host:port -> registered in Reactor
```

## Per request

```
client bytes
  -> Socket.read -> Connection.IoBuffer(in)
  -> RequestParser (+ ChunkedDecoder) -> HttpRequest
  -> Router (+ ServerConfig/LocationConfig) -> RequestContext
  -> HandlerFactory -> IRequestHandler.handle(ctx) -> HttpResponse
        | (if CGI) -> CgiProcess -> pipe -> CgiResponseParser -> HttpResponse
  -> ResponseSerializer -> Connection.IoBuffer(out)
  -> Socket.write -> client
```

## Direction of flow

- Config flows DOWN once at boot.
- Request flows UP per request (bytes -> meaning -> response).
- Response flows back DOWN (response -> bytes -> socket).

## CGI sub-flow (async, no blocking)

```
Handler picks CGI -> CgiProcess fork+exec
  parent registers cgi pipe fds in Reactor (non-block)
  Connection enters WAIT_CGI (no write yet)
  pipe writable -> feed request body to child stdin
  pipe readable -> collect child stdout
  child done -> CgiResponseParser -> HttpResponse
  Connection -> WRITING
```

## Error flow

```
any stage fails -> HttpError(code) -> ErrorResponse
  -> custom error_page or generated default -> WRITING
never crash. always respond.
```

## Buffer ownership

- `IoBuffer(in)` owned by Connection. Parser reads from it, consumes bytes.
- `IoBuffer(out)` owned by Connection. Serializer fills it, write drains it.
- Parser/handler never touch the socket directly.
