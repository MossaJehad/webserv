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
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Session &amp; Cookies — webserv</title>
    <style>
        * {{ box-sizing: border-box; margin: 0; padding: 0; }}
        body {{
            background-color: #090d16;
            color: #f8fafc;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 24px;
        }}
        .session-card {{
            background: #111827;
            border: 1px solid #1e293b;
            border-radius: 12px;
            padding: 40px 32px;
            max-width: 520px;
            width: 100%;
            text-align: center;
            box-shadow: 0 16px 36px rgba(0, 0, 0, 0.4);
        }}
        .badge-status {{
            display: inline-block;
            font-family: ui-monospace, "SF Mono", "Cascadia Code", "Fira Code", monospace;
            font-size: 0.75rem;
            font-weight: 700;
            color: #fbbf24;
            background: rgba(245, 158, 11, 0.15);
            border: 1px solid rgba(245, 158, 11, 0.3);
            padding: 4px 12px;
            border-radius: 20px;
            margin-bottom: 16px;
            letter-spacing: 0.05em;
        }}
        h1 {{
            font-size: 1.5rem;
            font-weight: 700;
            color: #f8fafc;
            margin-bottom: 20px;
            letter-spacing: -0.01em;
        }}
        .data-box {{
            background: #0b111e;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 16px;
            margin-bottom: 20px;
            text-align: left;
            font-family: ui-monospace, "SF Mono", "Cascadia Code", monospace;
            font-size: 0.85rem;
        }}
        .data-row {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 6px 0;
            border-bottom: 1px solid #1e293b;
        }}
        .data-row:last-child {{ border-bottom: none; }}
        .data-label {{ color: #94a3b8; }}
        .data-value {{ color: #38bdf8; font-weight: 600; }}
        .btn-group {{
            display: flex;
            justify-content: center;
            gap: 12px;
            margin-top: 24px;
        }}
        .btn {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            font-size: 0.9rem;
            font-weight: 600;
            padding: 10px 20px;
            border-radius: 6px;
            text-decoration: none;
            cursor: pointer;
            transition: background 0.15s ease, transform 0.1s ease;
        }}
        .btn-primary {{
            background: #0284c7;
            color: #ffffff;
        }}
        .btn-primary:hover {{ background: #0369a1; }}
        .btn-secondary {{
            background: #1a2234;
            color: #f8fafc;
            border: 1px solid #334155;
        }}
        .btn-secondary:hover {{ background: #25334c; }}
        .btn:active {{ transform: translateY(1px); }}
        .footer-note {{
            margin-top: 24px;
            font-size: 0.75rem;
            color: #64748b;
            font-family: ui-monospace, monospace;
        }}
    </style>
</head>
<body>
    <div class="session-card">
        <span class="badge-status">HTTP/1.1 COOKIE SESSION</span>
        <h1>Stateful Session Tracker</h1>
        
        <div class="data-box">
            <div class="data-row">
                <span class="data-label">Session UUID:</span>
                <span class="data-value">{session_id}</span>
            </div>
            <div class="data-row">
                <span class="data-label">Visit Counter:</span>
                <span class="data-value">{visits}</span>
            </div>
            <div class="data-row">
                <span class="data-label">Cookie Header:</span>
                <span class="data-value" style="font-size:0.75rem; color:#a855f7;">Set-Cookie: session_id, visits</span>
            </div>
        </div>

        <p style="color:#94a3b8; font-size:0.85rem; line-height:1.5;">Each refresh triggers CGI subprocess execution, reading <code>HTTP_COOKIE</code> and returning an incremented counter.</p>

        <div class="btn-group">
            <a href="/cgi-bin/session.py" class="btn btn-primary">Refresh (Visit +1)</a>
            <a href="/" class="btn btn-secondary">Return to Dashboard</a>
        </div>
        <div class="footer-note">webserv &bull; C++98 Non-blocking HTTP/1.1 Server</div>
    </div>
</body>
</html>""")
