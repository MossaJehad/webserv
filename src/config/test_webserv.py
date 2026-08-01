#!/usr/bin/env python3
"""
Socket-layer test suite for webserv.

Tests the *transport* behaviour only (accept / non-blocking I/O / poll
loop / cleanup) -- not HTTP semantics, since parsing isn't built yet.

Usage:
    python3 test_webserv.py [host] [port]
    python3 test_webserv.py 127.0.0.1 8080
"""

import socket
import sys
import time
import threading

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

PASS = []
FAIL = []


def report(name, ok, detail=""):
    if ok:
        PASS.append(name)
        print(f"[PASS] {name}")
    else:
        FAIL.append(name)
        print(f"[FAIL] {name}  {detail}")


def connect(timeout=3):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect((HOST, PORT))
    return s


# ---------------------------------------------------------------------
# 1. Basic sanity: a normal request gets a normal response.
# ---------------------------------------------------------------------
def test_basic_request():
    try:
        s = connect()
        s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
        data = s.recv(4096)
        s.close()
        report("basic_request", data.startswith(b"HTTP/1.1 200"), data[:50])
    except Exception as e:
        report("basic_request", False, str(e))


# ---------------------------------------------------------------------
# 2. Client connects and closes immediately without sending anything.
#    Server must not crash and must clean the fd up (checked indirectly
#    by later tests still working).
# ---------------------------------------------------------------------
def test_connect_then_immediate_close():
    try:
        s = connect()
        s.close()
        report("connect_then_immediate_close", True)
    except Exception as e:
        report("connect_then_immediate_close", False, str(e))


# ---------------------------------------------------------------------
# 3. Client sends a partial header block, then disconnects before
#    completing it (no \r\n\r\n ever arrives). Server should notice the
#    disconnect via poll() (recv() == 0) and clean up, not hang forever.
# ---------------------------------------------------------------------
def test_partial_then_disconnect():
    try:
        s = connect()
        s.sendall(b"GET / HTTP/1.1\r\nHost: x")  # no terminator
        s.close()
        report("partial_then_disconnect", True)
    except Exception as e:
        report("partial_then_disconnect", False, str(e))


# ---------------------------------------------------------------------
# 4. Malformed / garbage request line. The socket layer isn't parsing
#    HTTP yet, so it should still respond once it sees \r\n\r\n instead
#    of crashing on unexpected bytes.
# ---------------------------------------------------------------------
def test_malformed_request():
    try:
        s = connect()
        s.sendall(b"NOT-EVEN-HTTP \x00\x01\xff garbage\r\n\r\n")
        data = s.recv(4096)
        s.close()
        report("malformed_request_no_crash", len(data) > 0, data[:50])
    except Exception as e:
        report("malformed_request_no_crash", False, str(e))


# ---------------------------------------------------------------------
# 5. Trickle a request in one byte at a time with delays. Proves the
#    server accumulates across multiple non-blocking reads correctly
#    instead of assuming a full request arrives in one recv().
# ---------------------------------------------------------------------
def test_slow_trickle():
    try:
        s = connect(timeout=10)
        request = b"GET / HTTP/1.1\r\nHost: x\r\n\r\n"
        for byte in request:
            s.sendall(bytes([byte]))
            time.sleep(0.02)
        data = s.recv(4096)
        s.close()
        report("slow_trickle_bytes", data.startswith(b"HTTP/1.1 200"), data[:50])
    except Exception as e:
        report("slow_trickle_bytes", False, str(e))


# ---------------------------------------------------------------------
# 6. THE important one: a slow client that trickles data must NOT block
#    other clients from being served. This is the actual proof the
#    server is non-blocking / poll-driven rather than looping on one fd.
# ---------------------------------------------------------------------
def test_slow_client_does_not_block_others():
    slow_done = threading.Event()

    def slow_client():
        try:
            s = connect(timeout=10)
            s.sendall(b"GET / HTTP/1.1\r\n")
            time.sleep(2)  # deliberately hang mid-request
            s.sendall(b"Host: x\r\n\r\n")
            s.recv(4096)
            s.close()
        except Exception:
            pass
        finally:
            slow_done.set()

    t = threading.Thread(target=slow_client)
    t.start()
    time.sleep(0.3)  # let the slow client connect and start stalling

    try:
        start = time.time()
        s = connect(timeout=3)
        s.sendall(b"GET / HTTP/1.1\r\nHost: y\r\n\r\n")
        data = s.recv(4096)
        elapsed = time.time() - start
        s.close()
        ok = data.startswith(b"HTTP/1.1 200") and elapsed < 1.0
        report("slow_client_does_not_block_others", ok,
               f"elapsed={elapsed:.2f}s data={data[:30]}")
    except Exception as e:
        report("slow_client_does_not_block_others", False, str(e))

    t.join(timeout=5)


# ---------------------------------------------------------------------
# 7. Many concurrent connections at once (small stress test), all must
#    succeed with no hangs, no errors, no crash.
# ---------------------------------------------------------------------
def test_concurrent_connections(n=100):
    results = []
    lock = threading.Lock()

    def worker():
        try:
            s = connect(timeout=5)
            s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
            data = s.recv(4096)
            s.close()
            with lock:
                results.append(data.startswith(b"HTTP/1.1 200"))
        except Exception:
            with lock:
                results.append(False)

    threads = [threading.Thread(target=worker) for _ in range(n)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=10)

    ok = len(results) == n and all(results)
    report("concurrent_connections", ok, f"{sum(results)}/{n} succeeded")


# ---------------------------------------------------------------------
# 8. Open+close many sequential connections to catch fd leaks: if the
#    server leaks a fd per connection, this will eventually start
#    failing to accept/connect.
# ---------------------------------------------------------------------
def test_no_fd_leak(n=500):
    try:
        for _ in range(n):
            s = connect(timeout=3)
            s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
            s.recv(4096)
            s.close()
        report("no_fd_leak_after_n_sequential_conns", True, f"n={n}")
    except Exception as e:
        report("no_fd_leak_after_n_sequential_conns", False, str(e))


# ---------------------------------------------------------------------
# 9. Client half-closes its write side (shutdown) after headers -
#    another disconnect variant vs. a plain close().
# ---------------------------------------------------------------------
def test_shutdown_write_after_headers():
    try:
        s = connect()
        s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
        s.shutdown(socket.SHUT_WR)
        data = s.recv(4096)
        s.close()
        report("shutdown_write_after_headers", data.startswith(b"HTTP/1.1 200"), data[:50])
    except Exception as e:
        report("shutdown_write_after_headers", False, str(e))


# ---------------------------------------------------------------------
# 10. Oversized headers (no size-limit enforcement expected yet at this
#     layer -- just confirms the server doesn't crash on a large read).
# ---------------------------------------------------------------------
def test_large_headers():
    try:
        s = connect(timeout=5)
        junk_header = "X-Junk: " + ("a" * 8000) + "\r\n"
        s.sendall(f"GET / HTTP/1.1\r\nHost: x\r\n{junk_header}\r\n".encode())
        data = s.recv(4096)
        s.close()
        report("large_headers_no_crash", len(data) > 0, data[:50])
    except Exception as e:
        report("large_headers_no_crash", False, str(e))


def main():
    print(f"Testing webserv at {HOST}:{PORT}\n")

    test_basic_request()
    test_connect_then_immediate_close()
    test_partial_then_disconnect()
    test_malformed_request()
    test_slow_trickle()
    test_slow_client_does_not_block_others()
    test_shutdown_write_after_headers()
    test_large_headers()
    test_concurrent_connections(100)
    test_no_fd_leak(500)

    # Final check: server must still be responsive after everything above.
    test_basic_request()

    print(f"\n{len(PASS)} passed, {len(FAIL)} failed")
    if FAIL:
        print("Failed tests:", ", ".join(FAIL))
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()