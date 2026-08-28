#!/usr/bin/env python3
# Deliberately broken: this script raises before writing any CGI headers.
# The server must answer with an error status instead of relaying garbage
# or crashing.
raise RuntimeError("this CGI script fails on purpose")
