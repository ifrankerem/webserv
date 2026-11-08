#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qs

q = parse_qs(os.environ.get("QUERY_STRING", ""))
a = q.get("a", [""])[0]
b = q.get("b", [""])[0]
op = q.get("op", ["add"])[0]
symbols = {"add": "+", "sub": "-", "mul": "*", "div": "/"}

result = ""
if a and b:
    try:
        x, y = float(a), float(b)
        if op == "add":
            r = x + y
        elif op == "sub":
            r = x - y
        elif op == "mul":
            r = x * y
        elif y == 0:
            r = None
            result = "Cannot divide by zero."
        else:
            r = x / y
        if r is not None:
            result = "%g %s %g = %g" % (x, symbols.get(op, "?"), y, r)
    except ValueError:
        result = "You must enter numbers."


def option(value, label):
    selected = " selected" if value == op else ""
    return '<option value="%s"%s>%s</option>' % (value, selected, label)


print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Calculator (Python)</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Calculator</header>
    <main>
        <section>
            <h2>Calculator — Python</h2>
            <p class="muted">Running interpreter: Python %s</p>
            <form method="GET" action="/calc.py">
                <input name="a" value="%s" placeholder="a" size="8">
                <select name="op">%s%s%s%s</select>
                <input name="b" value="%s" placeholder="b" size="8">
                <button>Calculate</button>
            </form>
            <pre>%s</pre>
            <p><a href="/calc.php?%s">Do the same calculation in PHP</a></p>
        </section>
    </main>
</body>
</html>""" % (sys.version.split()[0], html.escape(a), option("add", "+"), option("sub", "-"),
               option("mul", "×"), option("div", "÷"), html.escape(b),
               html.escape(result or "The result will appear here."),
               html.escape(os.environ.get("QUERY_STRING", ""))))
