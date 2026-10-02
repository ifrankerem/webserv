#!/usr/bin/env python3
import os
print("Content-Type: text/plain")
print()
print("QUERY_STRING=" + os.environ.get("QUERY_STRING", ""))
