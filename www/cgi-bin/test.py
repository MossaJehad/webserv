#!/usr/bin/env python3
import sys
import os
import urllib.parse

method = os.environ.get("REQUEST_METHOD", "GET")
params = {}

if method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    body = sys.stdin.read(content_length) if content_length > 0 else ""
    parsed = urllib.parse.parse_qs(body)
    params = {k: v[0] for k, v in parsed.items()}
else:
    query_string = os.environ.get("QUERY_STRING", "")
    parsed = urllib.parse.parse_qs(query_string)
    params = {k: v[0] for k, v in parsed.items()}

import html

name = html.escape(params.get("name", "Student"))
status = html.escape(params.get("status", "OK"))

print("Content-Type: text/html\r\n\r\n", end="")
print(f"""<!DOCTYPE html>
<html>
<head><title>CGI Test</title></head>
<body>
    <h1>CGI Execution Succeeded</h1>
    <p>Method: {method}</p>
    <p>Name: {name}</p>
    <p>Status: {status}</p>
</body>
</html>""")
