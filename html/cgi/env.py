#!/usr/bin/env python3
import os
print("Content-Type: text/plain")
print()
print("METHOD=" + os.environ.get("REQUEST_METHOD", ""))
print("CTYPE=" + os.environ.get("CONTENT_TYPE", ""))
