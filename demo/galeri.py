#!/usr/bin/env python3
import html
import os
from urllib.parse import quote

UPLOAD_DIR = "uploads"
IMAGES = (".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico")

names = sorted(n for n in os.listdir(UPLOAD_DIR) if not n.startswith(".")) if os.path.isdir(UPLOAD_DIR) else []

items = ""
for name in names:
    url = "/uploads/" + quote(name)
    size = os.path.getsize(os.path.join(UPLOAD_DIR, name))
    if name.lower().endswith(IMAGES):
        preview = '<img src="%s" alt="%s">' % (html.escape(url), html.escape(name))
    else:
        preview = '<div class="file">dosya</div>'
    items += """<div class="card">
        <a href="%s">%s</a>
        <div class="muted">%s<br>%d bayt</div>
        <button type="button" onclick="removeFile('%s')">Sil (DELETE)</button>
    </div>""" % (html.escape(url), preview, html.escape(name), size, html.escape(url))
if not items:
    items = '<p class="muted">Henüz dosya yok.</p>'

print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Fotoğraf galerisi</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Fotoğraf galerisi</header>
    <main>
        <section>
            <h2>Yükle</h2>
            <p class="muted">Dosya <code>multipart/form-data</code> olarak <code>POST /uploads/</code> adresine gidiyor;
            sunucu onu <code>upload_store</code> klasörüne yazıp <b>201</b> dönüyor.
            Klasörün ham hali: <a href="/uploads/">/uploads/</a> (autoindex).</p>
            <form id="upload" method="POST" action="/uploads/" enctype="multipart/form-data">
                <input type="file" name="dosya" required>
                <button>Yükle</button>
            </form>
            <pre id="status">Sunucu cevabı burada görünecek.</pre>
        </section>
        <section>
            <h2>Galeri</h2>
            <div class="grid">%s</div>
        </section>
    </main>
    <script>
        const status = document.getElementById("status");
        document.getElementById("upload").addEventListener("submit", async (e) => {
            e.preventDefault();
            const res = await fetch("/uploads/", { method: "POST", body: new FormData(e.target) });
            status.textContent = "POST /uploads/ -> " + res.status + " " + res.statusText;
            if (res.ok) setTimeout(() => location.reload(), 800);
        });
        async function removeFile(url) {
            const res = await fetch(url, { method: "DELETE" });
            status.textContent = "DELETE " + decodeURIComponent(url) + " -> " + res.status + " " + res.statusText;
            if (res.ok) setTimeout(() => location.reload(), 800);
        }
    </script>
</body>
</html>""" % items)
