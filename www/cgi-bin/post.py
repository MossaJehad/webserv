#!/usr/bin/env python3
import sys
import os
import urllib.parse

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

parsed_data = urllib.parse.parse_qs(body)

print("Content-Type: text/html\r\n\r\n", end="")
print(f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI POST Echo</title>
    <style>
        body {{ font-family: sans-serif; background: #0f172a; color: #f8fafc; text-align: center; padding: 50px; }}
        .card {{ background: #1e293b; padding: 30px; border-radius: 10px; display: inline-block; max-width: 600px; }}
        pre {{ text-align: left; background: #0f172a; padding: 15px; border-radius: 6px; color: #38bdf8; }}
    </style>
</head>
<body>
    <div class="card">
        <h2>📥 CGI POST Received Data</h2>
        <p>Raw Stdin Length: <strong>{len(body)} bytes</strong></p>
        <h3>Parsed Fields:</h3>
        <pre>{parsed_data}</pre>
        <h3>Raw Body:</h3>
        <pre>{body}</pre>
        <br>
        <a href="/" style="color: #38bdf8;">Return Home</a>
    </div>
</body>
</html>""")
