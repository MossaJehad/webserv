#!/usr/bin/env python3
import os
import http.cookies
import uuid

raw_cookie = os.environ.get("HTTP_COOKIE", "")
cookie = http.cookies.SimpleCookie()
if raw_cookie:
    cookie.load(raw_cookie)

session_id = None
visits = 1

if "session_id" in cookie:
    session_id = cookie["session_id"].value
else:
    session_id = str(uuid.uuid4())[:8]

if "visits" in cookie:
    try:
        visits = int(cookie["visits"].value) + 1
    except ValueError:
        visits = 1

print(f"Set-Cookie: session_id={session_id}; Path=/; Max-Age=86400\r")
print(f"Set-Cookie: visits={visits}; Path=/; Max-Age=86400\r")
print("Content-Type: text/html\r\n\r\n", end="")

print(f"""<!DOCTYPE html>
<html>
<head>
    <title>Webserv Session & Cookies</title>
    <style>
        body {{ font-family: sans-serif; background: #0f172a; color: #f8fafc; text-align: center; padding: 50px; }}
        .card {{ background: #1e293b; padding: 30px; border-radius: 10px; display: inline-block; max-width: 550px; }}
        .highlight {{ color: #38bdf8; font-weight: bold; font-size: 20px; }}
        .badge {{ background: #0284c7; padding: 5px 12px; border-radius: 20px; font-size: 14px; }}
    </style>
</head>
<body>
    <div class="card">
        <h2>🍪 Session & Cookie Management</h2>
        <p>Session ID: <span class="badge">{session_id}</span></p>
        <p>You have visited this page <span class="highlight">{visits}</span> time(s) during this session.</p>
        <p><small>Refresh to see your visit count increase!</small></p>
        <br>
        <a href="/cgi-bin/session.py" style="display:inline-block; background:#0284c7; color:#fff; padding:8px 16px; border-radius:6px; text-decoration:none;">Refresh Page</a>
        <br><br>
        <a href="/" style="color: #94a3b8;">Return Home</a>
    </div>
</body>
</html>""")
