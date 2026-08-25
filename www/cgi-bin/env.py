#!/usr/bin/env python3
import os

print("Content-Type: text/html\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Environment Variables</title>
    <style>
        body { font-family: monospace; background: #0f172a; color: #f8fafc; padding: 30px; }
        table { width: 100%; border-collapse: collapse; background: #1e293b; border-radius: 8px; overflow: hidden; }
        th, td { text-align: left; padding: 8px 12px; border-bottom: 1px solid #334155; }
        th { background: #0284c7; color: #fff; }
        tr:hover { background: #334155; }
        .key { color: #38bdf8; font-weight: bold; }
    </style>
</head>
<body>
    <h2>🌍 CGI Environment Variables (RFC 3875)</h2>
    <table>
        <tr><th>Variable</th><th>Value</th></tr>""")

for key in sorted(os.environ.keys()):
    val = os.environ[key]
    print(f"<tr><td class='key'>{key}</td><td>{val}</td></tr>")

print("""    </table>
    <br><a href="/" style="color: #38bdf8;">Return Home</a>
</body>
</html>""")
