# Config Grammar (nginx-like)

Owner: Developer A. This documents the target grammar. No parser code here.

## Shape

```
server {
    listen        <host:port>;
    server_name   <name> [name...];
    client_max_body_size <size>;
    error_page    <code> <path>;

    location <path> {
        root        <dir>;
        index       <file>;
        autoindex   on|off;
        methods     <GET|POST|DELETE> [...];
        return      <code> <url>;        # redirect
        upload_dir  <dir>;
        cgi_ext     <.ext>;
        cgi_bin     <interpreter path>;
    }
}
```

## Rules

- Multiple `server` blocks allowed (virtual servers).
- Multiple `location` blocks per server. Match = longest path prefix.
- Server match = listen port + Host header vs server_name.
- Missing directive = default applied by `ConfigValidator`.
- Unknown directive / bad value = `ConfigError`, abort boot.

## Defaults (filled by ConfigValidator, not parser)

- autoindex off, methods = GET, index = index.html,
  client_max_body_size = sane default, default error pages generated.
