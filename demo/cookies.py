#!/usr/bin/env python3
import html
import json
import os
import re
import secrets
import sys
import time
from urllib.parse import parse_qs

SESSION_DIR = "/tmp/webserv_demo_sessions"
YEAR = 365 * 24 * 3600

env = os.environ
query = parse_qs(env.get("QUERY_STRING", ""))
action = query.get("action", [""])[0]
form = parse_qs(sys.stdin.buffer.read().decode("utf-8", "replace")) if env.get("REQUEST_METHOD") == "POST" else {}

raw_cookie = env.get("HTTP_COOKIE", "")
cookies = {}
for part in raw_cookie.split(";"):
    if "=" in part:
        name, value = part.strip().split("=", 1)
        cookies[name] = value

set_cookies = []
message = ""


def set_cookie(name, value, extra=""):
    set_cookies.append("%s=%s; Path=/%s" % (name, value, extra))
    cookies[name] = value


def delete_cookie(name):
    set_cookies.append("%s=; Path=/; Max-Age=0" % name)
    cookies.pop(name, None)


def session_file(sid):
    return os.path.join(SESSION_DIR, sid + ".json")


os.makedirs(SESSION_DIR, exist_ok=True)
sid = cookies.get("sid", "")
if not re.fullmatch(r"[0-9a-f]{32}", sid) or not os.path.exists(session_file(sid)):
    sid = ""
session = {}
if sid:
    with open(session_file(sid)) as f:
        session = json.load(f)

if action == "delete":
    for name in list(cookies):
        delete_cookie(name)
    if sid:
        os.remove(session_file(sid))
    sid, session = "", {}
    message = "All cookies were deleted and the session on the server was removed. Refresh the page to start a new session."
else:
    if not sid:
        sid = secrets.token_hex(16)
        session = {"created": time.strftime("%H:%M:%S"), "requests": 0}
        set_cookie("sid", sid, "; HttpOnly")
    session["requests"] = session.get("requests", 0) + 1

    if action == "accept":
        set_cookie("consent", "all", "; Max-Age=%d" % YEAR)
        set_cookie("theme", cookies.get("theme", "light"), "; Max-Age=%d" % YEAR)
        message = "All cookies were accepted. This single response sent more than one Set-Cookie line."
    elif action == "required":
        set_cookie("consent", "required", "; Max-Age=%d" % YEAR)
        message = "Only required cookies will be used."
    elif action == "withdraw":
        set_cookie("consent", "required", "; Max-Age=%d" % YEAR)
        for name in ("theme", "visits"):
            if name in cookies:
                delete_cookie(name)
        message = "Consent withdrawn: optional cookies were deleted, the session cookie stays."
    elif action == "theme":
        if cookies.get("consent") == "all":
            set_cookie("theme", "dark" if cookies.get("theme") == "light" else "light", "; Max-Age=%d" % YEAR)
        else:
            message = "The theme preference is an optional cookie; you must give consent first."
    elif action == "login":
        name = form.get("name", [""])[0].strip()[:40]
        if name:
            session["name"] = name
            message = "Logged in. The name is stored in the session on the server, not in a cookie."
    elif action == "logout":
        session.pop("name", None)
        message = "Logged out."

    if cookies.get("consent") == "all":
        visits = cookies.get("visits", "0")
        set_cookie("visits", str(int(visits) + 1 if visits.isdigit() else 1), "; Max-Age=%d" % YEAR)

    with open(session_file(sid), "w") as f:
        json.dump(session, f)


def rows(pairs):
    if not pairs:
        return '<p class="muted">(none)</p>'
    return "<table>" + "".join("<tr><th>%s</th><td>%s</td></tr>" % (html.escape(k), html.escape(v))
                               for k, v in pairs) + "</table>"


consent = cookies.get("consent", "")
dark = consent == "all" and cookies.get("theme") == "dark"
name = session.get("name", "")

if name:
    login = '<p>Welcome, <b>%s</b>! <a href="/cookies.py?action=logout">Log out</a></p>' % html.escape(name)
elif action != "delete":
    login = """<form method="POST" action="/cookies.py?action=login">
                <input name="name" placeholder="Your name" required><button>Log in</button>
            </form>"""
else:
    login = ""

banner = ""
if not consent and action != "delete":
    banner = """<div class="banner">
        <p><b>This site uses cookies.</b> A <b>required</b> cookie (<code>sid</code>) was already set for your session;
        we don't ask consent for it because the site does not work without it. We would like to use
        <b>optional</b> cookies for the theme preference and the visit counter.</p>
        <a class="button" href="/cookies.py?action=required">Required only</a>
        <a class="button" href="/cookies.py?action=accept">Accept all</a>
    </div>"""

headers = ["Content-Type: text/html; charset=UTF-8"] + ["Set-Cookie: " + c for c in set_cookies]
print("\r\n".join(headers) + "\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Cookies and sessions</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body class="%s">
    <header><a href="/">webserv demo</a> / Cookies and sessions</header>
    <main>
        %s
        <section>
            <h2>Session</h2>
            %s
            <p class="muted">Requests in this session: %d · Consent: <b>%s</b> · Theme: <b>%s</b> · Visit counter (cookie): <b>%s</b></p>
            <p>
                <a class="button" href="/cookies.py">Refresh</a>
                <a class="button" href="/cookies.py?action=theme">Change theme</a>
                <a class="button" href="/cookies.py?action=withdraw">Withdraw consent</a>
                <a class="button danger" href="/cookies.py?action=delete">Delete all cookies</a>
            </p>
        </section>
        <section>
            <h2>Cookie panel</h2>
            <h3>1. The <code>Cookie</code> header the browser sent with this request</h3>
            <pre>%s</pre>
            <h3>2. The <code>Set-Cookie</code> lines the server sent in this response</h3>
            <pre>%s</pre>
            <h3>3. The session stored on the server</h3>
            <pre>%s</pre>
            <h3>4. Cookies JavaScript can see (<code>document.cookie</code>)</h3>
            <pre id="jscookies"></pre>
            <p class="muted"><code>sid</code> is not shown here because it is <code>HttpOnly</code>: JavaScript cannot read it,
            so an XSS attack cannot steal it.</p>
        </section>
    </main>
    <script>
        document.getElementById("jscookies").textContent = document.cookie || "(empty)";
    </script>
</body>
</html>""" % (
    "dark" if dark else "",
    banner + ('<section class="note">%s</section>' % html.escape(message) if message else ""),
    login,
    session.get("requests", 0),
    html.escape(consent or "not chosen yet"),
    html.escape(cookies.get("theme", "-") if consent == "all" else "-"),
    html.escape(cookies.get("visits", "-")),
    html.escape("Cookie: " + raw_cookie if raw_cookie else "(no Cookie header: the browser has no cookies for this site)"),
    html.escape("\n".join("Set-Cookie: " + c for c in set_cookies) or "(no Set-Cookie in this response)"),
    html.escape(json.dumps(session, ensure_ascii=False, indent=2) if session else "(no session)"),
))
