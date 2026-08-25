#!/bin/bash
printf "Content-Type: text/html\r\n\r\n"
cat <<EOF
<!DOCTYPE html>
<html>
<head>
    <title>Shell CGI Response</title>
    <style>
        body { font-family: monospace; background: #0f172a; color: #4ade80; text-align: center; padding: 50px; }
        .box { background: #1e293b; padding: 25px; border-radius: 8px; display: inline-block; }
    </style>
</head>
<body>
    <div class="box">
        <h2>🐚 Bash Script CGI Execution</h2>
        <p>Executed by user: $(whoami)</p>
        <p>Server Time: $(date)</p>
        <p>Kernel: $(uname -s -r)</p>
        <br>
        <a href="/" style="color: #38bdf8;">Return Home</a>
    </div>
</body>
</html>
EOF
