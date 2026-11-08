#!/usr/bin/env python3
# For the CI timeout test (.github/workflows/ci.yml, "Timeout Tests").
# Ignores SIGTERM and waits forever without printing anything.
# After IDLE_TIMEOUT the server must SIGKILL this process and send 504 to the client.
import signal
import time

signal.signal(signal.SIGTERM, signal.SIG_IGN)
while True:
    time.sleep(1)
