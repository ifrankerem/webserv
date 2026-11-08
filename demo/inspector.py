#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qsl

env = os.environ
body = sys.stdin.buffer.read()
query = env.get("QUERY_STRING", "")
content_type = env.get("CONTENT_TYPE", "")


def esc(value):
    return html.escape(str(value))


def table(rows):
    if not rows:
        return '<p class="muted">(empty)</p>'
    out = "<table>"
    for name, value in rows:
        out += "<tr><th>" + esc(name) + "</th><td>" + esc(value) + "</td></tr>"
    return out + "</table>"


request_line = env.get("REQUEST_METHOD", "") + " " + env.get("SCRIPT_NAME", "")
if query:
    request_line += "?" + query
request_line += " " + env.get("SERVER_PROTOCOL", "")

headers = [(k[5:].replace("_", "-").title(), v) for k, v in sorted(env.items()) if k.startswith("HTTP_")]
if content_type:
    headers.append(("Content-Type", content_type))
if body:
    headers.append(("Content-Length", env.get("CONTENT_LENGTH", "")))

params = parse_qsl(query, keep_blank_values=True)
form = []
if content_type.startswith("application/x-www-form-urlencoded"):
    form = parse_qsl(body.decode("utf-8", "replace"), keep_blank_values=True)

preview = body[:4000].decode("utf-8", "replace")
if len(body) > 4000:
    preview += "\n... (" + str(len(body) - 4000) + " more bytes)"

print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Request inspector</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Request inspector</header>
    <main>""")

print("""        <section>
            <h2>Try it</h2>
            <form method="GET" action="/inspector.py">
                <input name="name" placeholder="name"><input name="message" placeholder="message">
                <button>Send GET</button>
            </form>
            <form method="POST" action="/inspector.py?source=form">
                <input name="name" placeholder="name"><input name="message" placeholder="message">
                <button>Send POST</button>
            </form>
        </section>""")

print("<section><h2>Request line</h2><pre>" + esc(request_line) + "</pre></section>")
print("<section><h2>Headers</h2>" + table(headers) + "</section>")
print("<section><h2>Query string parameters</h2>" + table(params) + "</section>")
print("<section><h2>Form fields (POST body)</h2>" + table(form) + "</section>")
print("<section><h2>Body (" + str(len(body)) + " bytes)</h2><pre>" + esc(preview) + "</pre></section>")
print("<section><h2>CGI environment variables</h2>" + table(sorted(env.items())) + "</section>")
print("    </main>\n</body>\n</html>")
