#!/usr/bin/env python3
with open("data.txt") as f:
    data = f.read().strip()
print("Content-Type: text/plain")
print()
print("cwd=" + data)
