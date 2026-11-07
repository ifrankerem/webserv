#!/usr/bin/env python3
# CI timeout testi icin (.github/workflows/ci.yml, "Timeout Tests").
# SIGTERM'i yok sayar ve hic cikti uretmeden sonsuza kadar bekler.
# Sunucu IDLE_TIMEOUT sonra bu sureci SIGKILL ile oldurup client'a 504 donmeli.
import signal
import time

signal.signal(signal.SIGTERM, signal.SIG_IGN)
while True:
    time.sleep(1)
