#!/usr/bin/env python3
import os
import urllib.parse

query_str = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query_str)

name = params.get("name", ["Guest"])[0]
score = params.get("score", ["0"])[0]

print("Content-Type: text/html\r\n\r\n", end="")
print(f"""<!DOCTYPE html>
<html>
<head>
    <title>Python CGI Response</title>
    <style>
        body {{ font-family: sans-serif; background: #0f172a; color: #f8fafc; text-align: center; padding: 50px; }}
        .card {{ background: #1e293b; padding: 30px; border-radius: 10px; display: inline-block; max-width: 500px; }}
        h1 {{ color: #38bdf8; }}
    </style>
</head>
<body>
    <div class="card">
        <h1>🐍 Python CGI Output</h1>
        <p>Hello, <strong>{name}</strong>!</p>
        <p>Your score is: <strong>{score}</strong></p>
        <p><small>Executed via Webserv CGI Engine</small></p>
        <br>
        <a href="/" style="color: #38bdf8;">Return Home</a>
    </div>
</body>
</html>""")
