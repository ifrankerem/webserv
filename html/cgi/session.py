#!/usr/bin/env python3
import os
import re
import secrets

SESSION_DIR = "/tmp/webserv_sessions"


def read_cookies():
    cookies = {}
    for part in os.environ.get("HTTP_COOKIE", "").split(";"):
        if "=" in part:
            name, value = part.strip().split("=", 1)
            cookies[name] = value
    return cookies


def session_path(sid):
    return os.path.join(SESSION_DIR, sid)


os.makedirs(SESSION_DIR, exist_ok=True)
sid = read_cookies().get("sid", "")
headers = ["Content-Type: text/html; charset=UTF-8"]

if "cikis=1" in os.environ.get("QUERY_STRING", "").split("&"):
    if re.fullmatch(r"[0-9a-f]{32}", sid) and os.path.exists(session_path(sid)):
        os.remove(session_path(sid))
    headers.append("Set-Cookie: sid=; Path=/; Max-Age=0")
    count = 0
    sid = ""
else:
    if not re.fullmatch(r"[0-9a-f]{32}", sid) or not os.path.exists(session_path(sid)):
        sid = secrets.token_hex(16)
        count = 0
        headers.append("Set-Cookie: sid=" + sid + "; Path=/; HttpOnly")
    else:
        with open(session_path(sid)) as f:
            count = int(f.read() or 0)
    count += 1
    with open(session_path(sid), "w") as f:
        f.write(str(count))

print("\r\n".join(headers) + "\r\n\r\n", end="")
if sid:
    print("<h1>Session</h1>")
    print("<p>Session id: <code>" + sid + "</code></p>")
    print("<p>Bu sayfayi bu oturumda <b>" + str(count) + "</b>. kez aciyorsun.</p>")
    print('<p><a href="session.py">Yenile</a> | <a href="session.py?cikis=1">Oturumu kapat</a></p>')
else:
    print("<h1>Oturum kapatildi</h1>")
    print('<p><a href="session.py">Yeni oturum ac</a></p>')
