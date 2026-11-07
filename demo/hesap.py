#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qs

q = parse_qs(os.environ.get("QUERY_STRING", ""))
a = q.get("a", [""])[0]
b = q.get("b", [""])[0]
op = q.get("op", ["topla"])[0]
symbols = {"topla": "+", "cikar": "-", "carp": "*", "bol": "/"}

result = ""
if a and b:
    try:
        x, y = float(a), float(b)
        if op == "topla":
            r = x + y
        elif op == "cikar":
            r = x - y
        elif op == "carp":
            r = x * y
        elif y == 0:
            r = None
            result = "Sıfıra bölünemez."
        else:
            r = x / y
        if r is not None:
            result = "%g %s %g = %g" % (x, symbols.get(op, "?"), y, r)
    except ValueError:
        result = "Sayı girmelisin."


def option(value, label):
    selected = " selected" if value == op else ""
    return '<option value="%s"%s>%s</option>' % (value, selected, label)


print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Hesap makinesi (Python)</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Hesap makinesi</header>
    <main>
        <section>
            <h2>Hesap makinesi — Python</h2>
            <p class="muted">Çalışan yorumlayıcı: Python %s</p>
            <form method="GET" action="/hesap.py">
                <input name="a" value="%s" placeholder="a" size="8">
                <select name="op">%s%s%s%s</select>
                <input name="b" value="%s" placeholder="b" size="8">
                <button>Hesapla</button>
            </form>
            <pre>%s</pre>
            <p><a href="/hesap.php?%s">Aynı hesabı PHP ile yap</a></p>
        </section>
    </main>
</body>
</html>""" % (sys.version.split()[0], html.escape(a), option("topla", "+"), option("cikar", "-"),
               option("carp", "×"), option("bol", "÷"), html.escape(b),
               html.escape(result or "Sonuç burada görünecek."),
               html.escape(os.environ.get("QUERY_STRING", ""))))
