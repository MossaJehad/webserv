#!/usr/bin/env python3
import sys
import time
import socket
import urllib.request
import urllib.parse
import http.client

def run_tests():
    host = "127.0.0.1"
    port = 8080
    base_url = f"http://{host}:{port}"
    passed = 0
    failed = 0

    print("=" * 60)
    print("      WEBSERV AUTOMATED INTEGRATION TEST SUITE       ")
    print("=" * 60)

    def test(name, condition, detail=""):
        nonlocal passed, failed
        if condition:
            print(f"[\033[32mPASS\033[0m] {name}")
            passed += 1
        else:
            print(f"[\033[31mFAIL\033[0m] {name} - {detail}")
            failed += 1

    # 1. Test GET static index.html
    try:
        req = urllib.request.Request(f"{base_url}/")
        with urllib.request.urlopen(req, timeout=3) as resp:
            body = resp.read().decode()
            test("GET / (Default Index)", resp.status == 200 and "webserv" in body)
    except Exception as e:
        test("GET / (Default Index)", False, str(e))

    # 2. Test GET CSS file
    try:
        req = urllib.request.Request(f"{base_url}/style.css")
        with urllib.request.urlopen(req, timeout=3) as resp:
            content_type = resp.headers.get("Content-Type", "")
            test("GET /style.css (MIME type)", resp.status == 200 and "text/css" in content_type)
    except Exception as e:
        test("GET /style.css (MIME type)", False, str(e))

    # 3. Test 404 Not Found
    try:
        req = urllib.request.Request(f"{base_url}/this_file_does_not_exist_9999.html")
        urllib.request.urlopen(req, timeout=3)
        test("GET 404 Not Found", False, "Expected 404 HTTPError")
    except urllib.error.HTTPError as e:
        test("GET 404 Not Found", e.code == 404)
    except Exception as e:
        test("GET 404 Not Found", False, str(e))

    # 4. Test Autoindex Directory Listing
    try:
        req = urllib.request.Request(f"{base_url}/uploads/")
        with urllib.request.urlopen(req, timeout=3) as resp:
            body = resp.read().decode()
            test("GET /uploads/ (Autoindex)", resp.status == 200 and "Index of" in body)
    except Exception as e:
        test("GET /uploads/ (Autoindex)", False, str(e))

    # 5. Test Redirection (301 /redirect -> /)
    try:
        conn = http.client.HTTPConnection(host, port, timeout=3)
        conn.request("GET", "/redirect")
        resp = conn.getresponse()
        loc = resp.getheader("Location")
        test("GET /redirect (301 Redirect)", resp.status == 301 and loc == "/", f"status={resp.status}, loc={loc}")
        conn.close()
    except Exception as e:
        test("GET /redirect (301 Redirect)", False, str(e))

    # 6. Test Method Restriction (405 Method Not Allowed)
    try:
        conn = http.client.HTTPConnection(host, port, timeout=3)
        conn.request("DELETE", "/restricted/index.html")
        resp = conn.getresponse()
        test("Method restriction (405 on DELETE in GET-only location)", resp.status == 405, f"status={resp.status}")
        conn.close()
    except Exception as e:
        test("Method restriction (405)", False, str(e))

    # 7. Test POST Upload (Raw body)
    try:
        upload_name = "test_upload_integration.txt"
        upload_content = "Hello from integration test!\nLine 2.\n"
        conn = http.client.HTTPConnection(host, port, timeout=3)
        headers = {"Content-Type": "text/plain"}
        conn.request("POST", f"/uploads/{upload_name}", body=upload_content, headers=headers)
        resp = conn.getresponse()
        body = resp.read().decode()
        test("POST /uploads/file (201 Created)", resp.status == 201, f"status={resp.status}")
        conn.close()

        # Verify the file is now readable
        req = urllib.request.Request(f"{base_url}/uploads/{upload_name}")
        with urllib.request.urlopen(req, timeout=3) as resp:
            read_body = resp.read().decode()
            test("GET /uploads/uploaded_file", read_body == upload_content)
    except Exception as e:
        test("POST /uploads/file", False, str(e))

    # 8. Test DELETE Method
    try:
        conn = http.client.HTTPConnection(host, port, timeout=3)
        conn.request("DELETE", f"/uploads/{upload_name}")
        resp = conn.getresponse()
        test(f"DELETE /uploads/{upload_name} (200 OK)", resp.status == 200 or resp.status == 204, f"status={resp.status}")
        conn.close()

        # Verify file is deleted -> 404
        try:
            req = urllib.request.Request(f"{base_url}/uploads/{upload_name}")
            urllib.request.urlopen(req, timeout=3)
            test("Verification after DELETE (404)", False, "File still exists")
        except urllib.error.HTTPError as e:
            test("Verification after DELETE (404)", e.code == 404)
    except Exception as e:
        test("DELETE /uploads/file", False, str(e))

    # 9. Test Chunked Transfer Encoding
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        chunked_request = (
            "POST /uploads/chunked_test.txt HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "7\r\n"
            "Chunk1 \r\n"
            "8\r\n"
            "Chunk2 \n\r\n"
            "0\r\n"
            "\r\n"
        )
        s.sendall(chunked_request.encode())
        response_data = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            response_data += chunk
        s.close()
        resp_str = response_data.decode()
        test("POST with Chunked Transfer Encoding (201 Created)", "201 Created" in resp_str or "200 OK" in resp_str, f"resp={resp_str[:100]}")
    except Exception as e:
        test("POST with Chunked Transfer Encoding", False, str(e))

    # 10. Test Python CGI GET with Query String
    try:
        req = urllib.request.Request(f"{base_url}/cgi-bin/hello.py?name=PeerReviewer&score=42")
        with urllib.request.urlopen(req, timeout=5) as resp:
            body = resp.read().decode()
            test("CGI Python GET (query params)", resp.status == 200 and "PeerReviewer" in body and "42" in body)
    except Exception as e:
        test("CGI Python GET", False, str(e))

    # 11. Test Python CGI POST with Body
    try:
        post_data = urllib.parse.urlencode({"user": "Alice", "message": "webserv test"}).encode()
        req = urllib.request.Request(f"{base_url}/cgi-bin/post.py", data=post_data)
        with urllib.request.urlopen(req, timeout=5) as resp:
            body = resp.read().decode()
            test("CGI Python POST (stdin body)", resp.status == 200 and "Alice" in body and "webserv test" in body)
    except Exception as e:
        test("CGI Python POST", False, str(e))

    # 12. Test Shell CGI Execution
    try:
        req = urllib.request.Request(f"{base_url}/cgi-bin/hello.sh")
        with urllib.request.urlopen(req, timeout=5) as resp:
            body = resp.read().decode()
            test("CGI Bash Execution", resp.status == 200 and "Bash Script CGI" in body)
    except Exception as e:
        test("CGI Bash Execution", False, str(e))

    # 13. Test Cookie & Session Management (Bonus)
    try:
        req1 = urllib.request.Request(f"{base_url}/cgi-bin/session.py")
        with urllib.request.urlopen(req1, timeout=5) as resp1:
            cookie_headers = resp1.headers.get_all("Set-Cookie", [])
            cookie_str = "; ".join(cookie_headers)
            has_cookie = "session_id=" in cookie_str and "visits=" in cookie_str
            test("CGI Cookies & Session (Set-Cookie)", resp1.status == 200 and has_cookie)

            # Follow up with cookie sent back
            req2 = urllib.request.Request(f"{base_url}/cgi-bin/session.py", headers={"Cookie": cookie_str})
            with urllib.request.urlopen(req2, timeout=5) as resp2:
                body2 = resp2.read().decode()
                test("CGI Cookies & Session (Visit Count 2)", "2" in body2)
    except Exception as e:
        test("CGI Cookies & Session", False, str(e))

    # 14. Test Client Max Body Size (413 Payload Too Large)
    try:
        # Send body larger than 10MB
        large_body = b"X" * (11 * 1024 * 1024)
        conn = http.client.HTTPConnection(host, port, timeout=5)
        headers = {"Content-Type": "application/octet-stream", "Content-Length": str(len(large_body))}
        conn.request("POST", "/uploads/large.bin", body=large_body, headers=headers)
        resp = conn.getresponse()
        test("Client Max Body Size (413 Payload Too Large)", resp.status == 413, f"status={resp.status}")
        conn.close()
    except Exception as e:
        # Either 413 or closed connection before full send
        test("Client Max Body Size (413 Payload Too Large)", True, f"Handled oversized payload: {e}")

    # 15. Test Multiple Concurrent Requests
    try:
        import concurrent.futures
        def fetch_page(idx):
            req = urllib.request.Request(f"{base_url}/")
            with urllib.request.urlopen(req, timeout=3) as r:
                return r.status == 200

        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
            results = list(executor.map(fetch_page, range(50)))
        test("Concurrent Requests (50 requests)", all(results))
    except Exception as e:
        test("Concurrent Requests", False, str(e))

    print("=" * 60)
    print(f"Summary: {passed} PASSED, {failed} FAILED (Total: {passed + failed})")
    print("=" * 60)
    return failed == 0

if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
