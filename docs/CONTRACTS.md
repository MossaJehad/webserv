# Shared Contracts

Frozen day 1. Co-signed by all 3 devs. Change only via PR + ping consumers.

## 1. Config data model (Owner A; consumed B + C)

`ServerConfig`
- host, ports[], server_names[], error_pages{code -> path},
  client_max_body_size, locations[].

`LocationConfig`
- path, root, index, autoindex(bool), methods[], redirect,
  upload_dir, cgi_ext, cgi_bin.

`ConfigTypes`
- Method enum {GET, POST, DELETE, UNKNOWN}, default limits, keywords.

## 2. Event interface (Owner B; implemented by Listener, Connection, CgiProcess)

`IEventHandler`
- onReadable(), onWritable(), fd(), wantsWrite(), isDead().
- Reactor talks ONLY through this. New fd types add here, loop unchanged.

## 3. HTTP messages (Owner C; HttpRequest consumed by routing/handlers, HttpResponse by Connection)

`HttpRequest` (data only)
- method, uri, path, query, version, headers, body, host.

`HttpResponse` (data only)
- status, headers, body, keepAlive.

## 4. Handler boundary (Owner C; consumed by Connection via HandlerFactory)

`RequestContext`
- request, matched ServerConfig, matched LocationConfig, resolved fs path.

`IRequestHandler`
- handle(RequestContext) returns HttpResponse.

## Ownership rule

Internal file of a module = owner edits freely.
Any field/signature above = shared contract = PR + notify both consumers.
