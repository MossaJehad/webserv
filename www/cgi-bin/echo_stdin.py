#!/usr/bin/env python3
"""Reads the request body until EOF, as the CGI specification expects.

The subject requires the server to un-chunk requests and let the CGI detect the
end of the body with EOF, so this script deliberately ignores CONTENT_LENGTH.
"""
import sys

body = sys.stdin.read()

print("Content-Type: text/plain\r\n\r\n", end="")
print(f"EOF_REACHED bytes={len(body)}")
print(f"BODY={body}")
