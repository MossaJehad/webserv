#!/usr/bin/env python3
"""Regression and robustness suite for webserv.

Covers the requirements from the subject that the main functional suite does not
exercise: resilience under load, correct behaviour on client disconnection,
accurate status codes on edge cases, and absence of leaked child processes.

Usage:
    ./webserv config/default.conf &
    python3 tests/integration/test_regression.py [--slow] [--oom] [--all]

--slow additionally runs the request-timeout test (~25s).
--oom  additionally runs the out-of-memory resilience test, which starts its own
       server on port 8099 under a hard address-space limit.
--all  runs everything.
"""

import os
import re
import socket
import subprocess
import sys
import time
import http.client
import concurrent.futures

HOST = "127.0.0.1"
PORT = 8080
BASE = f"http://{HOST}:{PORT}"

passed = 0
failed = 0
failures = []


def check(name, condition, detail=""):
    global passed, failed
    if condition:
        print(f"[\033[32mPASS\033[0m] {name}")
        passed += 1
    else:
        print(f"[\033[31mFAIL\033[0m] {name} - {detail}")
        failed += 1
        failures.append(name)


def raw_request(payload, read_all=True, timeout=5, linger=0.0):
    """Send raw bytes and return the raw response."""
    s = socket.create_connection((HOST, PORT), timeout=timeout)
    try:
        if isinstance(payload, str):
            payload = payload.encode()
        s.sendall(payload)
        if linger:
            time.sleep(linger)
        if not read_all:
            return b""
        data = b""
        while True:
            try:
                chunk = s.recv(65536)
            except socket.timeout:
                break
            if not chunk:
                break
            data += chunk
        return data
    finally:
        s.close()


def status_of(raw):
    m = re.match(rb"HTTP/1\.1 (\d{3})", raw)
    return int(m.group(1)) if m else None


def header_of(raw, name):
    head = raw.split(b"\r\n\r\n", 1)[0]
    for line in head.split(b"\r\n")[1:]:
        if line.lower().startswith(name.lower().encode() + b":"):
            return line.split(b":", 1)[1].strip().decode()
    return None


def server_pid():
    """PID of the running webserv process.

    Matches on /proc/<pid>/cmdline so it also works when the server is wrapped
    by a tool such as valgrind (whose process name is not "webserv"), while
    still ignoring shells whose command line merely mentions the binary.
    """
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open(f"/proc/{entry}/cmdline", "rb") as f:
                argv = f.read().split(b"\0")
        except OSError:
            continue
        argv = [a.decode(errors="replace") for a in argv if a]
        if not argv:
            continue
        if any(a.endswith("webserv") or a.endswith("/webserv") for a in argv) and \
                not any(a in ("bash", "sh", "-c") for a in argv):
            return entry
    return None


def child_processes(pid):
    if not pid:
        return []
    out = subprocess.run(["ps", "-eo", "pid,ppid,stat,comm"],
                         capture_output=True, text=True).stdout.splitlines()
    return [l for l in out[1:] if len(l.split()) >= 2 and l.split()[1] == pid]


# ---------------------------------------------------------------- HEAD semantics
def test_head_matches_get():
    get = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    head = raw_request(f"HEAD / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")

    get_len = header_of(get, "Content-Length")
    head_len = header_of(head, "Content-Length")
    body = head.split(b"\r\n\r\n", 1)[1] if b"\r\n\r\n" in head else b"?"

    check("HEAD reports the same Content-Length as GET",
          get_len is not None and get_len == head_len,
          f"GET={get_len} HEAD={head_len}")
    check("HEAD sends no payload", body == b"", f"body={body[:40]!r}")
    check("HEAD keeps the same Content-Type as GET",
          header_of(get, "Content-Type") == header_of(head, "Content-Type"))


# ------------------------------------------------------------ body size limits
def test_oversized_content_length_rejected_early():
    """A body larger than client_max_body_size must be refused from the headers
    alone, without the server buffering gigabytes first."""
    s = socket.create_connection((HOST, PORT), timeout=5)
    try:
        s.sendall((f"POST /uploads/huge.bin HTTP/1.1\r\nHost: {HOST}\r\n"
                   f"Content-Length: {4 * 1024 * 1024 * 1024}\r\n"
                   f"Content-Type: application/octet-stream\r\n\r\n").encode())
        # Not a single body byte is sent; the answer must still arrive.
        data = b""
        deadline = time.time() + 5
        while time.time() < deadline:
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                break
            data += chunk
            if b"\r\n\r\n" in data:
                break
        check("Oversized Content-Length answered 413 before reading a body",
              status_of(data) == 413, f"got {status_of(data)}")
    finally:
        s.close()


def test_oversized_chunked_body():
    """Chunked bodies over the limit must be 413, not 400."""
    body = b"X" * 65536
    payload = (f"POST /uploads/chunky.bin HTTP/1.1\r\nHost: {HOST}\r\n"
               f"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n").encode()
    # 11MB against a 10MB limit
    chunk_hdr = b"%x\r\n" % len(body)
    payload += (chunk_hdr + body + b"\r\n") * 176

    s = socket.create_connection((HOST, PORT), timeout=5)
    try:
        try:
            s.sendall(payload)
        except (BrokenPipeError, ConnectionResetError):
            pass  # server may answer and close before we finish sending
        data = b""
        deadline = time.time() + 5
        while time.time() < deadline:
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                break
            data += chunk
            if b"\r\n\r\n" in data:
                break
        check("Oversized chunked body answered 413 (not 400)",
              status_of(data) == 413, f"got {status_of(data)}")
    finally:
        s.close()


def test_rejected_upload_still_readable():
    """A client that streams a whole oversized body without watching for a reply
    must still be able to read the 413 instead of hitting a reset connection."""
    try:
        conn = http.client.HTTPConnection(HOST, PORT, timeout=25)
        conn.request("POST", "/uploads/over_limit.bin",
                     body=b"Z" * (12 * 1024 * 1024),
                     headers={"Content-Type": "application/octet-stream"})
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        check("Oversized upload response is readable by a naive client",
              resp.status == 413 and len(body) > 0,
              f"status={resp.status} body={len(body)}")
    except Exception as e:
        check("Oversized upload response is readable by a naive client", False,
              f"{type(e).__name__}: {e}")


def test_body_within_limit_accepted():
    body = b"Y" * (2 * 1024 * 1024)
    conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
    conn.request("POST", "/uploads/within_limit.bin", body=body,
                 headers={"Content-Type": "application/octet-stream"})
    resp = conn.getresponse()
    resp.read()
    conn.close()
    check("2MB body under the 10MB limit is accepted", resp.status == 201,
          f"got {resp.status}")

    conn = http.client.HTTPConnection(HOST, PORT, timeout=5)
    conn.request("DELETE", "/uploads/within_limit.bin")
    conn.getresponse().read()
    conn.close()


# ------------------------------------------------------------------- security
def test_path_traversal_blocked():
    for probe in ["/../Makefile", "/../../etc/passwd", "/%2e%2e/%2e%2e/Makefile",
                  "/uploads/../../Makefile"]:
        raw = raw_request(f"GET {probe} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
        body = raw.split(b"\r\n\r\n", 1)[1] if b"\r\n\r\n" in raw else b""
        leaked = b"CXXFLAGS" in body or b"root:x:" in body
        check(f"Path traversal blocked: {probe}", not leaked,
              f"status={status_of(raw)} leaked content")


def test_post_without_upload_dir_refused():
    raw = raw_request(f"POST /injected.html HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Content-Length: 5\r\nConnection: close\r\n\r\nBOOM!")
    check("POST to a location without upload_dir is refused",
          status_of(raw) in (403, 405), f"got {status_of(raw)}")
    check("No file was written into the document root",
          not os.path.exists("www/site/injected.html"))


# -------------------------------------------------------------- malformed input
def test_malformed_requests_do_not_crash():
    probes = [
        (b"\r\n\r\n", "bare CRLF"),
        (b"GARBAGE\r\n\r\n", "no method/uri/version"),
        (b"GET\r\n\r\n", "request line with one token"),
        (b"GET / HTTP/1.1\r\nBadHeaderNoColon\r\n\r\n", "header without colon"),
        (b"GET / HTTP/9.9\r\nHost: x\r\n\r\n", "bogus version"),
        (b"GET " + b"/" + b"a" * 9000 + b" HTTP/1.1\r\nHost: x\r\n\r\n", "over-long URI"),
        (b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\n", "non-numeric length"),
        (b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZZZ\r\n", "bad chunk size"),
        (b"\x00\x01\x02\x03\xff\xfe\r\n\r\n", "binary junk"),
        (b"GET / HTTP/1.1\r\nHost: x\r\n" + b"X-Pad: y\r\n" * 5000 + b"\r\n", "header flood"),
    ]
    for payload, label in probes:
        try:
            raw = raw_request(payload, timeout=4)
            code = status_of(raw)
            ok = code is not None and 400 <= code < 600
            # An immediate close without a response is also acceptable.
            ok = ok or raw == b""
            check(f"Malformed input handled: {label}", ok, f"resp={raw[:60]!r}")
        except (ConnectionResetError, BrokenPipeError, socket.timeout) as e:
            check(f"Malformed input handled: {label}", True, str(e))

    # Server must still be serving afterwards
    raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Server still healthy after malformed input barrage",
          status_of(raw) == 200, f"got {status_of(raw)}")


def test_abrupt_disconnects():
    """Clients that vanish mid-request must not disturb the server."""
    for i in range(40):
        s = socket.create_connection((HOST, PORT), timeout=3)
        s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n")  # incomplete on purpose
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                     b"\x01\x00\x00\x00\x00\x00\x00\x00")  # force RST
        s.close()

    # And clients that disconnect while a CGI script is still running
    for i in range(10):
        s = socket.create_connection((HOST, PORT), timeout=3)
        s.sendall(f"GET /cgi-bin/timeout.py HTTP/1.1\r\nHost: {HOST}\r\n\r\n".encode())
        time.sleep(0.05)
        s.close()

    time.sleep(1)
    raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Server survives abrupt client disconnects", status_of(raw) == 200,
          f"got {status_of(raw)}")


# ------------------------------------------------------------- process hygiene
def test_no_zombie_children():
    pid = server_pid()
    check("Server process located for child inspection", pid is not None)
    if not pid:
        return

    for i in range(25):
        raw_request(f"GET /cgi-bin/hello.py?i={i} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")

    time.sleep(1.5)
    kids = child_processes(pid)
    zombies = [k for k in kids if "Z" in k.split()[2]]
    check("No zombie CGI children after 25 CGI requests", not zombies,
          f"zombies={zombies}")
    check("No lingering CGI children after 25 CGI requests", not kids,
          f"children={kids}")


def test_fd_not_leaking():
    pid = server_pid()
    if not pid:
        check("Server process located for fd inspection", False)
        return

    def count_fds():
        try:
            return len(os.listdir(f"/proc/{pid}/fd"))
        except OSError:
            return -1

    before = count_fds()
    for i in range(60):
        raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
        raw_request(f"GET /cgi-bin/hello.py HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    time.sleep(1.5)
    after = count_fds()
    check("File descriptors are not leaking",
          before > 0 and after <= before + 2, f"before={before} after={after}")


# ------------------------------------------------------------------ keep-alive
def test_keep_alive_and_pipelining():
    s = socket.create_connection((HOST, PORT), timeout=5)
    try:
        for i in range(5):
            s.sendall(f"GET / HTTP/1.1\r\nHost: {HOST}\r\n\r\n".encode())
            data = b""
            while b"\r\n\r\n" not in data:
                data += s.recv(65536)
            length = int(header_of(data, "Content-Length"))
            body = data.split(b"\r\n\r\n", 1)[1]
            while len(body) < length:
                body += s.recv(65536)
            if i == 0:
                check("Keep-alive: first response on reused socket",
                      status_of(data) == 200)
        check("Keep-alive: 5 sequential requests on one connection", True)
    except Exception as e:
        check("Keep-alive: 5 sequential requests on one connection", False, str(e))
    finally:
        s.close()

    # Two requests written in a single packet must both be answered
    try:
        s = socket.create_connection((HOST, PORT), timeout=5)
        s.sendall((f"GET / HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
                   f"GET /style.css HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n").encode())
        data = b""
        while True:
            chunk = s.recv(65536)
            if not chunk:
                break
            data += chunk
        s.close()
        statuses = re.findall(rb"HTTP/1\.1 (\d{3}) ", data)
        check("Pipelined requests both answered",
              statuses == [b"200", b"200"],
              f"statuses={statuses}")
    except Exception as e:
        check("Pipelined requests both answered", False, str(e))


def test_connection_close_honoured():
    raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Connection: close is echoed back",
          (header_of(raw, "Connection") or "").lower() == "close",
          header_of(raw, "Connection"))


# ---------------------------------------------------------------- stress tests
def test_stress_sequential():
    ok = 0
    start = time.time()
    total = 400
    for i in range(total):
        try:
            raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
            if status_of(raw) == 200:
                ok += 1
        except Exception:
            pass
    elapsed = time.time() - start
    check(f"Stress: {total} sequential requests all served "
          f"({ok}/{total}, {total / max(elapsed, 0.001):.0f} req/s)",
          ok == total, f"{ok}/{total}")


def test_stress_concurrent():
    total, workers = 300, 50

    def one(i):
        try:
            raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n",
                              timeout=10)
            return status_of(raw) == 200
        except Exception:
            return False

    start = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
        results = list(ex.map(one, range(total)))
    elapsed = time.time() - start
    ok = sum(results)
    check(f"Stress: {total} concurrent requests over {workers} workers "
          f"({ok}/{total}, {total / max(elapsed, 0.001):.0f} req/s)",
          ok == total, f"{ok}/{total}")


def test_stress_mixed_workload():
    def job(i):
        kind = i % 4
        try:
            if kind == 0:
                raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n", timeout=15)
                return status_of(raw) == 200
            if kind == 1:
                raw = raw_request(f"GET /cgi-bin/hello.py?i={i} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n", timeout=15)
                return status_of(raw) == 200
            if kind == 2:
                body = f"payload-{i}".encode()
                raw = raw_request((f"POST /uploads/stress_{i}.txt HTTP/1.1\r\nHost: {HOST}\r\n"
                                   f"Content-Length: {len(body)}\r\nConnection: close\r\n\r\n").encode() + body,
                                  timeout=15)
                return status_of(raw) == 201
            raw = raw_request(f"GET /nope_{i} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n", timeout=15)
            return status_of(raw) == 404
        except Exception:
            return False

    with concurrent.futures.ThreadPoolExecutor(max_workers=30) as ex:
        results = list(ex.map(job, range(200)))
    ok = sum(results)
    check(f"Stress: mixed static/CGI/upload/404 workload ({ok}/200)", ok == 200, f"{ok}/200")

    # tidy up
    for i in range(200):
        if i % 4 == 2:
            try:
                raw_request(f"DELETE /uploads/stress_{i}.txt HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
            except Exception:
                pass


def test_many_idle_connections():
    """Holding many sockets open must not stop the server serving others."""
    idle = []
    try:
        for _ in range(120):
            try:
                s = socket.create_connection((HOST, PORT), timeout=3)
                idle.append(s)
            except Exception:
                break
        check(f"Accepted {len(idle)} simultaneous idle connections", len(idle) >= 100,
              f"only {len(idle)}")
        raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
        check("Still serving while many idle connections are held",
              status_of(raw) == 200, f"got {status_of(raw)}")
    finally:
        for s in idle:
            s.close()


# ------------------------------------------------------------------- slow tests
def test_cgi_stdin_eof():
    """The subject requires the CGI to detect the end of the body with EOF, and
    chunked requests must be un-chunked before reaching it."""
    body = b"A" * 1000
    raw = raw_request((f"POST /cgi-bin/echo_stdin.py HTTP/1.1\r\nHost: {HOST}\r\n"
                       f"Content-Length: {len(body)}\r\nConnection: close\r\n\r\n").encode() + body,
                      timeout=15)
    check("CGI reading stdin until EOF gets the whole body",
          status_of(raw) == 200 and f"EOF_REACHED bytes={len(body)}".encode() in raw,
          f"status={status_of(raw)} resp={raw[-120:]!r}")

    # Same body, chunked: the server must un-chunk it and still deliver EOF.
    chunked = (f"POST /cgi-bin/echo_stdin.py HTTP/1.1\r\nHost: {HOST}\r\n"
               f"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n").encode()
    chunked += b"3e8\r\n" + body + b"\r\n0\r\n\r\n"
    raw = raw_request(chunked, timeout=15)
    check("Chunked request is un-chunked and CGI sees EOF",
          status_of(raw) == 200 and f"EOF_REACHED bytes={len(body)}".encode() in raw,
          f"status={status_of(raw)} resp={raw[-120:]!r}")


def test_cgi_does_not_inherit_server_sockets():
    """A CGI child holding the listening socket keeps the port bound after the
    server dies, and holding a client socket delays that peer's shutdown."""
    pid = server_pid()
    if not pid:
        check("Server process located for fd inheritance check", False)
        return

    s = socket.create_connection((HOST, PORT), timeout=20)
    try:
        s.sendall(f"GET /cgi-bin/timeout.py HTTP/1.1\r\nHost: {HOST}\r\n\r\n".encode())
        time.sleep(0.6)

        kids = [l.split()[0] for l in child_processes(pid)]
        check("A CGI child is running for inspection", bool(kids), f"children={kids}")
        if not kids:
            return

        sockets = 0
        for kid in kids:
            try:
                for fd in os.listdir(f"/proc/{kid}/fd"):
                    target = os.readlink(f"/proc/{kid}/fd/{fd}")
                    if target.startswith("socket:"):
                        sockets += 1
            except OSError:
                pass
        check("CGI child inherits no server sockets (FD_CLOEXEC)", sockets == 0,
              f"{sockets} socket(s) inherited")
    finally:
        s.close()


def test_concurrent_cgi_never_starves():
    """Regression: a closed client's descriptor number can be recycled by a new
    CGI pipe inside the same poll cycle. The connection sweep must not then
    unregister that pipe by fd number, or the CGI is never read and the request
    only ends at the CGI timeout."""
    def one(i):
        t0 = time.time()
        try:
            raw = raw_request(f"GET /cgi-bin/hello.py?i={i} HTTP/1.1\r\nHost: {HOST}\r\n"
                              f"Connection: close\r\n\r\n", timeout=20)
            return status_of(raw), time.time() - t0
        except Exception:
            return None, time.time() - t0

    worst = 0.0
    bad = []
    for _ in range(4):
        with concurrent.futures.ThreadPoolExecutor(max_workers=30) as ex:
            results = list(ex.map(one, range(60)))
        for st, lat in results:
            worst = max(worst, lat)
            if st != 200:
                bad.append(st)

    check(f"240 concurrent CGI requests all answered (worst latency {worst:.2f}s)",
          not bad, f"non-200 responses: {bad}")
    # A trivial CGI must never approach the CGI timeout.
    check("No CGI request was starved into a timeout", worst < 5.0,
          f"worst={worst:.2f}s")


def test_delete_does_not_wipe_directories():
    """DELETE addresses one resource; it must never recursively destroy a tree."""
    victim = "www/uploads/regr_victim"
    os.makedirs(victim, exist_ok=True)
    with open(os.path.join(victim, "keep.txt"), "w") as f:
        f.write("must survive")

    raw = raw_request(f"DELETE /uploads/regr_victim/ HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Connection: close\r\n\r\n")
    check("DELETE on a directory is refused", status_of(raw) in (403, 409),
          f"got {status_of(raw)}")
    check("Directory and its contents survived the DELETE",
          os.path.exists(os.path.join(victim, "keep.txt")))

    import shutil
    shutil.rmtree(victim, ignore_errors=True)

    # A regular file must still be deletable.
    raw = raw_request((f"POST /uploads/regr_del.txt HTTP/1.1\r\nHost: {HOST}\r\n"
                       f"Content-Length: 4\r\nConnection: close\r\n\r\ndata").encode())
    raw = raw_request(f"DELETE /uploads/regr_del.txt HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Connection: close\r\n\r\n")
    check("DELETE on a regular file still works", status_of(raw) in (200, 204),
          f"got {status_of(raw)}")


def test_control_characters_rejected():
    """NUL and other control bytes must not reach paths or response headers."""
    raw = raw_request(b"GET /\x00evil HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n")
    head = raw.split(b"\r\n\r\n", 1)[0]
    check("NUL byte in the request target is rejected", status_of(raw) == 400,
          f"got {status_of(raw)}")
    check("No NUL byte is reflected into response headers", b"\x00" not in head)

    raw = raw_request(b"GET /a\x07b HTTP/1.1\r\nHost: h\r\nConnection: close\r\n\r\n")
    check("Control byte in the request target is rejected", status_of(raw) == 400,
          f"got {status_of(raw)}")


def test_empty_cgi_output_is_bad_gateway():
    """RFC 3875 6.2: a script response needs at least Content-Type or Location."""
    script = "www/cgi-bin/regr_empty.py"
    with open(script, "w") as f:
        f.write("#!/usr/bin/env python3\nimport sys\nsys.exit(0)\n")
    os.chmod(script, 0o755)
    try:
        raw = raw_request(f"GET /cgi-bin/regr_empty.py HTTP/1.1\r\nHost: {HOST}\r\n"
                          f"Connection: close\r\n\r\n", timeout=15)
        check("CGI that produces no output yields 502, not an empty 200",
              status_of(raw) == 502, f"got {status_of(raw)}")
    finally:
        os.remove(script)


def test_empty_post_body_refused():
    raw = raw_request(f"POST /uploads/ HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Content-Length: 0\r\nConnection: close\r\n\r\n")
    check("POST with an empty body does not create a file",
          status_of(raw) == 400, f"got {status_of(raw)}")
    stray = [f for f in os.listdir("www/uploads") if f.startswith("upload_")]
    check("No auto-named placeholder file was created", not stray, f"stray={stray}")


def test_cgi_cannot_inject_headers():
    """A script's output is untrusted: control bytes in its header fields must
    never reach the wire, or it could truncate or forge response headers."""
    script = "www/cgi-bin/regr_nulhdr.py"
    with open(script, "w") as f:
        f.write("#!/usr/bin/env python3\nimport os\n"
                'os.write(1, b"Content-Type: text/html\\r\\n"\n'
                '            b"X-Inject: before\\x00after\\r\\n\\r\\n<html>ok</html>")\n')
    os.chmod(script, 0o755)
    try:
        raw = raw_request(f"GET /cgi-bin/regr_nulhdr.py HTTP/1.1\r\nHost: {HOST}\r\n"
                          f"Connection: close\r\n\r\n", timeout=15)
        head = raw.split(b"\r\n\r\n", 1)[0]
        check("CGI cannot emit a NUL byte into response headers",
              b"\x00" not in head, "NUL reached the response head")
        check("Malformed CGI header field is dropped, not relayed",
              b"X-Inject" not in head, "bad field was relayed")
        check("Valid part of the CGI response is still served",
              status_of(raw) == 200 and b"text/html" in head.lower(),
              f"status={status_of(raw)}")
    finally:
        os.remove(script)


def test_autoindex_escapes_filenames():
    """A file name is attacker-controlled (uploads), so the listing must escape
    it instead of injecting raw markup."""
    name = "<img src=x onerror=alert(1)>.txt"
    path = os.path.join("www/uploads", name)
    with open(path, "w") as f:
        f.write("x")
    try:
        raw = raw_request(f"GET /uploads/ HTTP/1.1\r\nHost: {HOST}\r\n"
                          f"Connection: close\r\n\r\n")
        check("Autoindex does not inject raw markup from a file name",
              b"<img src=x onerror" not in raw)
        check("Autoindex HTML-escapes the file name",
              b"&lt;img src=x onerror" in raw)
    finally:
        os.remove(path)


def test_strict_request_syntax():
    """RFC 7230 requirements that keep hostile bytes out of the pipeline."""
    cases = [
        ("duplicate Host header rejected (RFC 7230 5.4)",
         b"GET / HTTP/1.1\r\nHost: localhost\r\nHost: evil.com\r\nConnection: close\r\n\r\n", 400),
        ("control character in a header value rejected",
         b"GET / HTTP/1.1\r\nHost: localhost\r\nX-T: \x01v\r\nConnection: close\r\n\r\n", 400),
        ("whitespace inside a header name rejected",
         b"GET / HTTP/1.1\r\nHost: localhost\r\nX Bad: v\r\nConnection: close\r\n\r\n", 400),
        ("extra token in the request line rejected",
         b"GET / HTTP/1.1 extra\r\nHost: localhost\r\nConnection: close\r\n\r\n", 400),
        ("well-formed request still accepted",
         b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", 200),
    ]
    for label, payload, expected in cases:
        raw = raw_request(payload)
        check(label, status_of(raw) == expected,
              f"expected {expected}, got {status_of(raw)}")


def test_out_of_memory_resilience():
    """The subject requires the server not to crash "even if it runs out of
    memory". This spawns a second server on another port under a hard
    address-space cap and hammers it with bodies it cannot possibly allocate.

    Run explicitly with --oom because it needs its own server instance.
    """
    port = 8099
    conf = "/tmp/webserv_oom.conf"
    with open(conf, "w") as f:
        f.write("server {\n"
                f"    listen 127.0.0.1:{port};\n"
                "    client_max_body_size 100M;\n"
                "    root www/site;\n"
                "    location / { root www/site; methods GET; }\n"
                "    location /uploads { root www/uploads; methods GET POST;"
                " upload_dir www/uploads; }\n"
                "}\n")

    # ulimit must be applied in the child before exec, hence the shell wrapper.
    proc = subprocess.Popen(["/bin/sh", "-c",
                             f"ulimit -v 65536; exec ./webserv {conf}"],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        time.sleep(1.5)
        if proc.poll() is not None:
            check("Capped server started for the OOM test", False,
                  "it exited immediately")
            return

        def flood(i):
            try:
                s = socket.create_connection((HOST, port), timeout=25)
                body = b"Z" * (9 * 1024 * 1024)
                s.sendall((f"POST /uploads/oom_{i}.bin HTTP/1.1\r\nHost: {HOST}\r\n"
                           f"Content-Length: {len(body)}\r\n"
                           f"Connection: close\r\n\r\n").encode())
                s.sendall(body)
                while s.recv(65536):
                    pass
                s.close()
            except Exception:
                pass  # a dropped connection is the expected, acceptable outcome

        with concurrent.futures.ThreadPoolExecutor(max_workers=10) as ex:
            list(ex.map(flood, range(10)))
        time.sleep(1)

        check("Server survives allocation failure without terminating",
              proc.poll() is None, f"process exited with {proc.poll()}")

        # And it must still serve ordinary traffic afterwards.
        try:
            s = socket.create_connection((HOST, port), timeout=10)
            s.sendall(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n".encode())
            data = b""
            while True:
                chunk = s.recv(65536)
                if not chunk:
                    break
                data += chunk
            s.close()
            check("Server still answers normal requests after an OOM burst",
                  status_of(data) == 200, f"got {status_of(data)}")
        except Exception as e:
            check("Server still answers normal requests after an OOM burst",
                  False, f"{type(e).__name__}: {e}")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except Exception:
            proc.kill()
        for leftover in os.listdir("www/uploads"):
            if leftover.startswith("oom_"):
                os.remove(os.path.join("www/uploads", leftover))
        if os.path.exists(conf):
            os.remove(conf)


def test_multiple_cgi_types():
    """Bonus: CGI dispatch is driven by the configured extension, so several
    interpreters must work side by side."""
    for script, language in (("hello.py", "Python"), ("hello.sh", "Bash"),
                             ("hello.pl", "Perl")):
        if not os.path.exists(os.path.join("www/cgi-bin", script)):
            continue
        raw = raw_request(f"GET /cgi-bin/{script} HTTP/1.1\r\nHost: {HOST}\r\n"
                          f"Connection: close\r\n\r\n", timeout=15)
        check(f"CGI type works: {language} ({script})", status_of(raw) == 200,
              f"got {status_of(raw)}")


def test_pipelined_flood_is_bounded():
    """Regression: while a CGI runs, the client socket is drained so a
    disconnect is noticed. Draining defeats TCP backpressure, so the stash must
    be capped or a peer could grow server memory without limit."""
    pid = server_pid()
    if not pid:
        check("Server process located for the flood test", False)
        return

    def rss_kb():
        try:
            with open(f"/proc/{pid}/status") as f:
                for line in f:
                    if line.startswith("VmRSS:"):
                        return int(line.split()[1])
        except OSError:
            pass
        return -1

    before = rss_kb()

    def abuse(i):
        try:
            s = socket.create_connection((HOST, PORT), timeout=20)
            s.sendall(f"GET /cgi-bin/timeout.py HTTP/1.1\r\nHost: {HOST}\r\n\r\n".encode())
            time.sleep(0.3)
            chunk = b"X" * 65536
            for _ in range(200):      # ~13MB per client, far past the cap
                s.sendall(chunk)
            s.close()
            return False              # never stopped: the cap did not hold
        except Exception:
            return True               # refused, which is the intended outcome

    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as ex:
        stopped = list(ex.map(abuse, range(5)))

    time.sleep(1)
    after = rss_kb()

    check("Peers streaming during CGI are cut off", all(stopped),
          f"{stopped.count(False)} of 5 streamed unchecked")
    check(f"Memory stayed bounded under the flood (+{after - before} KB)",
          before > 0 and after - before < 51200,
          f"before={before}KB after={after}KB")

    raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Server still serving after the flood", status_of(raw) == 200,
          f"got {status_of(raw)}")


def test_method_status_codes():
    """The evaluation sheet penalises inaccurate status codes. RFC 7231
    distinguishes a method the server does not implement (501) from a known
    method that this route forbids (405, which must advertise Allow)."""
    raw = raw_request(f"FOOBAR / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Unrecognised method returns 501 Not Implemented",
          status_of(raw) == 501, f"got {status_of(raw)}")
    check("501 response does not claim an Allow list",
          header_of(raw, "Allow") is None, header_of(raw, "Allow"))

    raw = raw_request(f"DELETE /restricted/index.html HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Connection: close\r\n\r\n")
    allow = header_of(raw, "Allow") or ""
    check("Known but forbidden method returns 405",
          status_of(raw) == 405, f"got {status_of(raw)}")
    check(f"405 advertises Allow ({allow!r}) per RFC 7231 6.5.5",
          "GET" in allow, f"Allow={allow!r}")

    # The server must stay healthy after an unknown method.
    raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
    check("Server healthy after an unknown method", status_of(raw) == 200)


def test_virtual_hosts_share_a_port():
    """Two server blocks on one interface:port must be dispatched by Host."""
    raw_a = raw_request(f"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
    raw_b = raw_request(f"GET / HTTP/1.1\r\nHost: not-configured.invalid\r\n"
                        f"Connection: close\r\n\r\n")
    check("Request with a configured Host is served", status_of(raw_a) == 200,
          f"got {status_of(raw_a)}")
    check("Request with an unknown Host falls back to the default server",
          status_of(raw_b) == 200, f"got {status_of(raw_b)}")


def test_cgi_env_path_info_and_script_name():
    """RFC 3875 4.1.13: SCRIPT_NAME identifies the script and must not include
    PATH_INFO. These exact values were changed, so assert them directly rather
    than relying on method/query coverage."""
    raw = raw_request(f"GET /cgi-bin/env.py/extra/path?k=v&n=2 HTTP/1.1\r\n"
                      f"Host: {HOST}\r\nConnection: close\r\n\r\n", timeout=15)
    check("CGI with PATH_INFO is executed", status_of(raw) == 200,
          f"got {status_of(raw)}")

    body = raw.split(b"\r\n\r\n", 1)[1] if b"\r\n\r\n" in raw else b""
    expectations = [
        ("SCRIPT_NAME", "/cgi-bin/env.py"),
        ("PATH_INFO", "/extra/path"),
        ("QUERY_STRING", "k=v&n=2"),
        ("REQUEST_URI", "/cgi-bin/env.py/extra/path?k=v&n=2"),
    ]
    for var, expected in expectations:
        # env.py renders "VAR ... value"; look for the pair on one line.
        found = None
        for line in body.decode(errors="replace").splitlines():
            if var in line:
                found = line
                break
        check(f"CGI env {var} == {expected}",
              found is not None and expected in found,
              f"line={found!r}")


def test_pipelined_after_chunked_body():
    """Regression: the chunked decoder consumed the whole read buffer, so a
    request arriving in the same packet as the terminal chunk was swallowed."""
    payload = (b"POST /uploads/regr_chunked.txt HTTP/1.1\r\nHost: " + HOST.encode() +
               b"\r\nTransfer-Encoding: chunked\r\n\r\n"
               b"5\r\nhello\r\n0\r\n\r\n"
               b"GET / HTTP/1.1\r\nHost: " + HOST.encode() +
               b"\r\nConnection: close\r\n\r\n")
    raw = raw_request(payload, timeout=10)
    # Count real status lines: the served HTML mentions "HTTP/1.1" in its text,
    # so a plain substring count would over-report.
    statuses = re.findall(rb"HTTP/1\.1 (\d{3}) ", raw)
    check("Request pipelined behind a chunked body is still answered",
          len(statuses) == 2,
          f"got statuses {statuses}, expected 2 responses")

    raw_request(f"DELETE /uploads/regr_chunked.txt HTTP/1.1\r\nHost: {HOST}\r\n"
                f"Connection: close\r\n\r\n")


def test_chunked_requires_crlf():
    """RFC 7230 4.1: chunk framing uses CRLF. Accepting a bare LF would frame
    the body differently from stricter proxies, which enables smuggling."""
    payload = (b"POST /uploads/regr_lf.txt HTTP/1.1\r\nHost: " + HOST.encode() +
               b"\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n"
               b"5\r\nhello\n0\r\n\r\n")          # bare LF after the chunk data
    raw = raw_request(payload, timeout=10)
    check("Chunk terminated with a bare LF is rejected",
          status_of(raw) == 400, f"got {status_of(raw)}")
    check("No file was written from the malformed chunked body",
          not os.path.exists("www/uploads/regr_lf.txt"))


def test_cgi_cannot_forge_status_line():
    """A control byte in the CGI Status reason phrase would be copied into our
    status line, forging or truncating the response head."""
    script = "www/cgi-bin/regr_status.py"
    with open(script, "w") as f:
        f.write("#!/usr/bin/env python3\nimport os\n"
                'os.write(1, b"Status: 200 OK\\rX-Forged: injected\\r\\n"\n'
                '            b"Content-Type: text/html\\r\\n\\r\\n<p>hi</p>")\n')
    os.chmod(script, 0o755)
    try:
        raw = raw_request(f"GET /cgi-bin/regr_status.py HTTP/1.1\r\nHost: {HOST}\r\n"
                          f"Connection: close\r\n\r\n", timeout=15)
        head = raw.split(b"\r\n\r\n", 1)[0]
        status_line = head.split(b"\r\n")[0]
        check("CGI cannot inject a header via the Status reason phrase",
              b"X-Forged" not in head, f"head={head[:120]!r}")
        check("Status line contains no embedded CR",
              b"\r" not in status_line, f"status_line={status_line!r}")
    finally:
        os.remove(script)


def test_head_on_error_has_no_body():
    """HEAD semantics apply to error responses too, not just successful ones."""
    # An oversized Content-Length is answered from the parser via sendError().
    raw = raw_request(f"HEAD /uploads/x.bin HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Content-Length: {4 * 1024 * 1024 * 1024}\r\n"
                      f"Connection: close\r\n\r\n")
    body = raw.split(b"\r\n\r\n", 1)[1] if b"\r\n\r\n" in raw else b""
    check("HEAD error response carries no payload", body == b"",
          f"status={status_of(raw)} body={body[:60]!r}")

    raw = raw_request(f"HEAD /no-such-file HTTP/1.1\r\nHost: {HOST}\r\n"
                      f"Connection: close\r\n\r\n")
    body = raw.split(b"\r\n\r\n", 1)[1] if b"\r\n\r\n" in raw else b""
    check("HEAD 404 carries no payload but keeps its status",
          status_of(raw) == 404 and body == b"",
          f"status={status_of(raw)} body={body[:60]!r}")


def test_request_timeout_408():
    """An incomplete request must be timed out rather than hang forever."""
    s = socket.create_connection((HOST, PORT), timeout=40)
    try:
        s.sendall(f"POST /uploads/slow.txt HTTP/1.1\r\nHost: {HOST}\r\n"
                  f"Content-Length: 1000\r\n\r\npartial".encode())
        start = time.time()
        data = b""
        while True:
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                break
            data += chunk
        elapsed = time.time() - start
        code = status_of(data)
        # A silent close would also "not hang", so requiring 408 specifically is
        # what stops the response behaviour from regressing unnoticed.
        check(f"Stalled request is answered with 408 (~{elapsed:.0f}s)",
              code == 408, f"status={code} resp={data[:80]!r}")
        check("Stalled request did not hang indefinitely", elapsed < 35,
              f"{elapsed:.0f}s")
    finally:
        s.close()


def main():
    slow = "--slow" in sys.argv
    oom = "--oom" in sys.argv or "--all" in sys.argv
    if "--all" in sys.argv:
        slow = True

    print("=" * 62)
    print("      WEBSERV REGRESSION & ROBUSTNESS SUITE")
    print("=" * 62)

    tests = [
        test_head_matches_get,
        test_oversized_content_length_rejected_early,
        test_oversized_chunked_body,
        test_rejected_upload_still_readable,
        test_body_within_limit_accepted,
        test_path_traversal_blocked,
        test_post_without_upload_dir_refused,
        test_delete_does_not_wipe_directories,
        test_control_characters_rejected,
        test_empty_cgi_output_is_bad_gateway,
        test_empty_post_body_refused,
        test_cgi_cannot_inject_headers,
        test_autoindex_escapes_filenames,
        test_strict_request_syntax,
        test_method_status_codes,
        test_virtual_hosts_share_a_port,
        test_cgi_env_path_info_and_script_name,
        test_pipelined_after_chunked_body,
        test_chunked_requires_crlf,
        test_cgi_cannot_forge_status_line,
        test_head_on_error_has_no_body,
        test_malformed_requests_do_not_crash,
        test_abrupt_disconnects,
        test_no_zombie_children,
        test_fd_not_leaking,
        test_cgi_stdin_eof,
        test_cgi_does_not_inherit_server_sockets,
        test_concurrent_cgi_never_starves,
        test_pipelined_flood_is_bounded,
        test_multiple_cgi_types,
        test_keep_alive_and_pipelining,
        test_connection_close_honoured,
        test_many_idle_connections,
        test_stress_sequential,
        test_stress_concurrent,
        test_stress_mixed_workload,
    ]
    if slow:
        tests.append(test_request_timeout_408)
    if oom:
        tests.append(test_out_of_memory_resilience)

    for t in tests:
        try:
            t()
        except Exception as e:
            check(t.__name__, False, f"unexpected exception: {e!r}")

    # Final liveness assertion: the server must still be up after everything.
    try:
        raw = raw_request(f"GET / HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n")
        check("Server still available at end of suite", status_of(raw) == 200)
    except Exception as e:
        check("Server still available at end of suite", False, str(e))

    print("=" * 62)
    print(f"Summary: {passed} PASSED, {failed} FAILED (Total: {passed + failed})")
    if failures:
        print("Failed: " + ", ".join(failures))
    print("=" * 62)
    return failed == 0


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
