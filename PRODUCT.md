# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users
- **42 Evaluators & Peer Reviewers**: Need immediate, friction-free verification that every mandatory and bonus requirement (methods, chunked encoding, multipart uploads, CGI, session cookies, error handling) is operational.
- **Systems & Backend Developers**: Testing edge-case HTTP behaviors, large payload handling, timeout resilience, and non-blocking I/O concurrency.

## Product Purpose
Provide a fully compliant, high-performance C++98 HTTP/1.1 web server and an integrated frontend testbed dashboard that lets developers and evaluators inspect server health and verify HTTP semantics, CGI execution, storage mutations, and session state in real time.

## Positioning
A zero-dependency, standalone HTTP/1.1 testbed and developer console running directly on a custom non-blocking C++98 `poll()` reactor, designed specifically for rigorous RFC verification and 100% offline 42 curriculum peer evaluation.

## Operating Context
- Evaluation defense environments without reliable internet access, requiring zero external CDNs, fonts, or npm build steps.
- Direct TCP/HTTP interactions via browser UI, curl, and automated Python integration test suites.
- Non-blocking I/O multiplexing across client sockets and CGI pipes on Linux/macOS.

## Capabilities and Constraints
- **Zero Build Dependencies**: Plain vanilla HTML5, CSS3, and JavaScript only. No npm, webpack, Tailwind, or external scripts.
- **RFC Conformance**: Strict support for HTTP/1.1 (RFC 7230, RFC 7231) methods (`GET`, `POST`, `DELETE`, `HEAD`), chunked transfer decoding, persistent connections, and status codes.
- **Common Gateway Interface**: RFC 3875 compliant asynchronous non-blocking CGI execution (`.py`, `.sh`, `.php`) with process timeout cleanup.
- **File Management**: Direct upload handling via `POST` (`multipart/form-data` and raw body) and file deletion via `DELETE`.
- **Session Tracking**: Cookie-based session tracking (`Set-Cookie` / `Cookie`) demonstrating stateful interactions across requests.
- **Virtual Hosts & Routing**: Multi-port listening, route matching, redirects (301/302), and custom error page templates (400, 403, 404, 405, 413, 500, 502, 504).

## Brand Commitments
- **Name**: `webserv`
- **Aesthetic Persona**: Modern Systems Console / High-Precision Developer Dashboard. Engineered, clean, dark slate surfaces, high-contrast semantic badges, and monospace metadata.

## Evidence on Hand
- C++98 reactor and server implementation in `src/` compiled to `./webserv`.
- Configuration files in `config/` (`default.conf`, `multi_port.conf`, `cgi.conf`).
- Static web interface and assets in `www/site/`.
- Working CGI test scripts in `www/cgi-bin/` (`hello.py`, `env.py`, `post.py`, `hello.sh`, `session.py`).
- Automated Python integration test suite in `tests/integration/test_webserv.py`.
- Custom error pages in `www/errors/`.

## Product Principles
- **100% Offline Integrity**: Never depend on external network assets; everything required to render and interact must be self-contained in the repository.
- **Frictionless Verification**: Every HTTP method, status code, error page, and CGI handler must be demonstrable and inspectable within 1-2 clicks.
- **Strict RFC Fidelity**: Frontend behaviors must faithfully exercise real HTTP/1.1 semantics and accurately expose backend status and response headers.
- **Zero-Cruft Precision**: Avoid decorative fluff or generic SaaS tropes; prioritize clear telemetry, fast response times, and readable monospace outputs.
