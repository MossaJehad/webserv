#!/usr/bin/env python3
"""Proves the CGI child runs in the script's own directory.

It opens a neighbouring file by relative path only. If the server did not
chdir() into www/cgi-bin before exec, the open fails and the script says so.
"""

import os
import sys

print("Content-Type: text/html\r")
print("\r")

print("<!DOCTYPE html><html><head><title>CGI working directory</title></head>")
print("<body style=\"font-family: Arial, sans-serif;\">")
print("<h1>CGI relative path access</h1>")
print("<p>Working directory: <code>%s</code></p>" % os.getcwd())

try:
    with open("relative_data.txt") as handle:          # relative path on purpose
        print("<p>Read <code>relative_data.txt</code> successfully:</p>")
        print("<pre>%s</pre>" % handle.read().strip())
        print("<p><strong>RELATIVE_PATH_OK</strong></p>")
except IOError as exc:
    print("<p><strong>RELATIVE_PATH_FAILED</strong>: %s</p>" % exc)
    sys.stdout.flush()
    sys.exit(0)

print("</body></html>")
