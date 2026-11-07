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
action = query.get("islem", [""])[0]
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

if action == "sil":
    for name in list(cookies):
        delete_cookie(name)
    if sid:
        os.remove(session_file(sid))
    sid, session = "", {}
    message = "Bütün cookie'ler silindi ve sunucudaki oturum kaldırıldı. Sayfayı yenileyince yeni bir oturum açılır."
else:
    if not sid:
        sid = secrets.token_hex(16)
        session = {"olusturuldu": time.strftime("%H:%M:%S"), "istek_sayisi": 0}
        set_cookie("sid", sid, "; HttpOnly")
    session["istek_sayisi"] = session.get("istek_sayisi", 0) + 1

    if action == "kabul":
        set_cookie("consent", "hepsi", "; Max-Age=%d" % YEAR)
        set_cookie("tema", cookies.get("tema", "acik"), "; Max-Age=%d" % YEAR)
        message = "Bütün cookie'ler kabul edildi. Bu tek cevapta birden fazla Set-Cookie satırı gönderildi."
    elif action == "zorunlu":
        set_cookie("consent", "zorunlu", "; Max-Age=%d" % YEAR)
        message = "Sadece zorunlu cookie'ler kullanılacak."
    elif action == "geri":
        set_cookie("consent", "zorunlu", "; Max-Age=%d" % YEAR)
        for name in ("tema", "ziyaret"):
            if name in cookies:
                delete_cookie(name)
        message = "İzin geri çekildi: isteğe bağlı cookie'ler silindi, oturum cookie'si duruyor."
    elif action == "tema":
        if cookies.get("consent") == "hepsi":
            set_cookie("tema", "koyu" if cookies.get("tema") == "acik" else "acik", "; Max-Age=%d" % YEAR)
        else:
            message = "Tema tercihi isteğe bağlı bir cookie; önce izin vermelisin."
    elif action == "giris":
        name = form.get("isim", [""])[0].strip()[:40]
        if name:
            session["isim"] = name
            message = "Giriş yapıldı. İsim cookie'de değil, sunucudaki oturumda saklanıyor."
    elif action == "cikis":
        session.pop("isim", None)
        message = "Çıkış yapıldı."

    if cookies.get("consent") == "hepsi":
        visits = cookies.get("ziyaret", "0")
        set_cookie("ziyaret", str(int(visits) + 1 if visits.isdigit() else 1), "; Max-Age=%d" % YEAR)

    with open(session_file(sid), "w") as f:
        json.dump(session, f)


def rows(pairs):
    if not pairs:
        return '<p class="muted">(yok)</p>'
    return "<table>" + "".join("<tr><th>%s</th><td>%s</td></tr>" % (html.escape(k), html.escape(v))
                               for k, v in pairs) + "</table>"


consent = cookies.get("consent", "")
dark = consent == "hepsi" and cookies.get("tema") == "koyu"
name = session.get("isim", "")

if name:
    login = '<p>Hoş geldin, <b>%s</b>! <a href="/cookies.py?islem=cikis">Çıkış yap</a></p>' % html.escape(name)
elif action != "sil":
    login = """<form method="POST" action="/cookies.py?islem=giris">
                <input name="isim" placeholder="İsmin" required><button>Giriş yap</button>
            </form>"""
else:
    login = ""

banner = ""
if not consent and action != "sil":
    banner = """<div class="banner">
        <p><b>Bu site cookie kullanıyor.</b> Oturumun için <b>zorunlu</b> bir cookie (<code>sid</code>) zaten konuldu;
        site onsuz çalışmadığı için bunun için izin istemiyoruz. Tema tercihi ve ziyaret sayacı için
        <b>isteğe bağlı</b> cookie'ler kullanmak istiyoruz.</p>
        <a class="button" href="/cookies.py?islem=zorunlu">Sadece zorunlu</a>
        <a class="button" href="/cookies.py?islem=kabul">Hepsini kabul et</a>
    </div>"""

headers = ["Content-Type: text/html; charset=UTF-8"] + ["Set-Cookie: " + c for c in set_cookies]
print("\r\n".join(headers) + "\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Cookie ve oturum</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body class="%s">
    <header><a href="/">webserv demo</a> / Cookie ve oturum</header>
    <main>
        %s
        <section>
            <h2>Oturum</h2>
            %s
            <p class="muted">Bu oturumdaki istek sayısı: %d · İzin durumu: <b>%s</b> · Tema: <b>%s</b> · Ziyaret sayacı (cookie): <b>%s</b></p>
            <p>
                <a class="button" href="/cookies.py">Yenile</a>
                <a class="button" href="/cookies.py?islem=tema">Temayı değiştir</a>
                <a class="button" href="/cookies.py?islem=geri">İzni geri çek</a>
                <a class="button danger" href="/cookies.py?islem=sil">Bütün cookie'leri sil</a>
            </p>
        </section>
        <section>
            <h2>Cookie paneli</h2>
            <h3>1. Tarayıcının bu istekte gönderdiği <code>Cookie</code> header'ı</h3>
            <pre>%s</pre>
            <h3>2. Sunucunun bu cevapta gönderdiği <code>Set-Cookie</code> satırları</h3>
            <pre>%s</pre>
            <h3>3. Sunucuda saklanan oturum (session)</h3>
            <pre>%s</pre>
            <h3>4. JavaScript'in görebildiği cookie'ler (<code>document.cookie</code>)</h3>
            <pre id="jscookies"></pre>
            <p class="muted"><code>sid</code> burada görünmüyor çünkü <code>HttpOnly</code>: JavaScript okuyamıyor,
            bu yüzden bir XSS saldırısıyla çalınamıyor.</p>
        </section>
    </main>
    <script>
        document.getElementById("jscookies").textContent = document.cookie || "(boş)";
    </script>
</body>
</html>""" % (
    "koyu" if dark else "",
    banner + ('<section class="note">%s</section>' % html.escape(message) if message else ""),
    login,
    session.get("istek_sayisi", 0),
    html.escape(consent or "henüz seçilmedi"),
    html.escape(cookies.get("tema", "-") if consent == "hepsi" else "-"),
    html.escape(cookies.get("ziyaret", "-")),
    html.escape("Cookie: " + raw_cookie if raw_cookie else "(Cookie header'ı yok: tarayıcının bu siteye ait cookie'si yok)"),
    html.escape("\n".join("Set-Cookie: " + c for c in set_cookies) or "(bu cevapta Set-Cookie yok)"),
    html.escape(json.dumps(session, ensure_ascii=False, indent=2) if session else "(oturum yok)"),
))
