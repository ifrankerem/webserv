#!/usr/bin/env python3
import os, sys
length = int(os.environ.get("CONTENT_LENGTH", "0"))
body = sys.stdin.read(length)
print("Content-Type: text/plain")
print()
print("received:" + body)
