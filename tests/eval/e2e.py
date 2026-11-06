#!/usr/bin/env python3
"""EVAL_RULES.md uctan uca testleri: gercek ./webserv sureci, gercek soketler.

Repo kokunden, `make` sonrasi calistir:
    python3 tests/eval/e2e.py              # Katman 1: C, H, CGI, R-01, IO-07/08, P (her push)
    python3 tests/eval/e2e.py --heavy      # R-02 timeout'lar, IO-09, CGI-08, fd/bellek limiti
    python3 tests/eval/e2e.py --siege      # S-01..S-04 (siege gerekli)
    python3 tests/eval/e2e.py --valgrind   # Katman 1 valgrind altinda + S-05 sizinti raporu
    python3 tests/eval/e2e.py --bonus      # B-01 cookie/session, B-02 ikinci CGI turu (php-cgi gerekli)

Sadece standart kutuphane. Her satir bir kural ID'si tasir; ayrintisi EVAL_RULES.md'de.
"""
import glob
import hashlib
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.getcwd()
BIN = os.path.join(ROOT, "webserv")
IDLE = int(os.environ.get("IDLE_TIMEOUT", "30"))  # sunucunun checkTimeouts() suresi (sn)
MODE = sys.argv[1] if len(sys.argv) > 1 else ""
SLOW = 10 if MODE == "--valgrind" else 1
D = tempfile.mkdtemp(prefix="webserv-eval-")
WRAP = []
if MODE == "--valgrind":
    WRAP = ["valgrind", "--leak-check=full", "--show-leak-kinds=all",
            f"--log-file={D}/vg-%p.log"]

P = 18080          # ana server (error_page 404 var)
P_LIMIT = 18081    # client_max_body_size 20
P_PLAIN = 18082    # error_page yok, server_name yok
P_VHOST = 18084    # ayni port, iki server_name
results = []


def report(rid, ok, msg=""):
    ok = None if ok is None else bool(ok)
    tag = {True: "PASS", False: "FAIL", None: "WARN"}[ok]
    print(f"[{tag}] {rid}: {msg}", flush=True)
    results.append((rid, ok))


# ---------------------------------------------------------------- HTTP istemcisi

class Resp:
    def __init__(self, status=None, headers=None, body=b"", error=None, head=b""):
        self.status, self.headers, self.body, self.error, self.head = status, headers or {}, body, error, head

    def __repr__(self):
        return f"<{self.status} {self.error or ''} {self.body[:60]!r}>"


def read_response(s, head_only=False):
    buf = b""
    while b"\r\n\r\n" not in buf:
        d = s.recv(65536)
        if not d:
            return Resp(error="baglanti yanitsiz kapandi" if not buf else "eksik header", head=buf)
        buf += d
    head, _, rest = buf.partition(b"\r\n\r\n")
    lines = head.split(b"\r\n")
    m = re.match(rb"HTTP/1\.[01] (\d{3})", lines[0])
    if not m:
        return Resp(error=f"gecersiz status satiri {lines[0][:60]!r}", head=head)
    headers = {}
    for l in lines[1:]:
        k, _, v = l.partition(b":")
        headers[k.strip().lower().decode()] = v.strip().decode(errors="replace")
    if head_only:
        return Resp(int(m.group(1)), headers, b"", head=head)
    if "content-length" in headers:
        need = int(headers["content-length"])
        while len(rest) < need:
            d = s.recv(1 << 20)
            if not d:
                break
            rest += d
        body = rest[:need]
    else:
        while True:
            d = s.recv(1 << 20)
            if not d:
                break
            rest += d
        body = rest
    return Resp(int(m.group(1)), headers, body, head=head)


def raw(data, port=P, ip="127.0.0.1", timeout=10, head_only=False):
    try:
        with socket.create_connection((ip, port), timeout=timeout * SLOW) as s:
            s.sendall(data)
            return read_response(s, head_only)
    except OSError as e:
        return Resp(error=f"{type(e).__name__}: {e}")


def chunk(body, size=4096):
    out = b"".join(b"%x\r\n%s\r\n" % (len(body[i:i + size]), body[i:i + size])
                   for i in range(0, len(body), size))
    return out + b"0\r\n\r\n"


def http(method, path, port=P, ip="127.0.0.1", body=None, headers=None, chunked=False,
         host="localhost", timeout=10):
    h = {"Host": host, "Connection": "close"}
    h.update(headers or {})
    data = b""
    if body is not None:
        if chunked:
            h["Transfer-Encoding"] = "chunked"
            data = chunk(body)
        else:
            h["Content-Length"] = str(len(body))
            data = body
    req = f"{method} {path} HTTP/1.1\r\n" + "".join(f"{k}: {v}\r\n" for k, v in h.items()) + "\r\n"
    return raw(req.encode() + data, port, ip, timeout, head_only=(method == "HEAD"))


# ---------------------------------------------------------------- sunucu sureci

class Server:
    def __init__(self, conf, wrap=None):
        self.conf, self.wrap = conf, wrap if wrap is not None else WRAP
        self.proc = None

    def start(self, ports=(P,)):
        self.log = open(os.path.join(D, f"server-{time.time():.0f}.log"), "w")
        self.proc = subprocess.Popen(self.wrap + [BIN, self.conf], stdin=subprocess.DEVNULL,
                                     stdout=self.log, stderr=subprocess.STDOUT, cwd=ROOT)
        deadline = time.time() + 10 * SLOW
        for port in ports:
            while time.time() < deadline:
                try:
                    socket.create_connection(("127.0.0.1", port), timeout=1).close()
                    break
                except OSError:
                    if self.proc.poll() is not None:
                        return False
                    time.sleep(0.1)
        return self.proc.poll() is None

    def alive(self, port=P):
        return self.proc.poll() is None and http("GET", "/", port=port).status == 200

    def stop(self):
        if self.proc and self.proc.poll() is None:
            self.proc.send_signal(signal.SIGINT)
            try:
                self.proc.wait(15 * SLOW)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        return self.proc.returncode

    def fds(self):
        try:
            return len(os.listdir(f"/proc/{self.proc.pid}/fd"))
        except OSError:
            return -1

    def children(self):
        kids = []
        for st in glob.glob("/proc/[0-9]*/stat"):
            try:
                parts = open(st).read().rsplit(")", 1)[1].split()
                if int(parts[1]) == self.proc.pid:
                    kids.append((st.split("/")[2], parts[0]))  # pid, durum (Z = zombi)
            except (OSError, IndexError, ValueError):
                pass
        return kids


# ---------------------------------------------------------------- fixture + config

CGI = {
    "hello.py": 'import sys\nsys.stdout.write("Content-Type: text/plain\\r\\n\\r\\nCGI-HELLO")\n',
    "env.py": 'import os,sys\nsys.stdout.write("Content-Type: text/plain\\r\\n\\r\\n")\n'
              'for k,v in sorted(os.environ.items()): sys.stdout.write(k+"="+v+"\\n")\n',
    "body.py": 'import sys,hashlib\nb=sys.stdin.buffer.read()\n'
               'sys.stdout.write("Content-Type: text/plain\\r\\n\\r\\nlen=%d md5=%s" % (len(b), hashlib.md5(b).hexdigest()))\n',
    "stream.py": 'import sys,time\nsys.stdout.write("Content-Type: application/octet-stream\\r\\n\\r\\n")\n'
                 'for i in range(80):\n    sys.stdout.write("x"*65536); sys.stdout.flush()\n'
                 '    if i % 10 == 0: time.sleep(0.02)\n',
    "big.py": 'import sys\nsys.stdout.write("Content-Type: text/plain\\r\\n\\r\\n")\n'
              'for i in range(160): sys.stdout.write("y"*65536)\n',
    "cwd.py": 'import sys\nsys.stdout.write("Content-Type: text/plain\\r\\n\\r\\n"+open("data.txt").read())\n',
    "slow.py": 'import sys,time\ntime.sleep(3)\nsys.stdout.write("Content-Type: text/plain\\r\\n\\r\\nSLOW")\n',
    "hang.py": 'import signal\nsignal.signal(signal.SIGTERM, signal.SIG_IGN)\nwhile True: pass\n',
    "syntax.py": 'def (:\n',
    "exit1.py": 'import sys\nsys.exit(1)\n',
    "badheader.py": 'import sys\nsys.stdout.write("bu bir header degil\\n\\ngovde")\n',
    "earlyclose.py": 'import os,sys,time,signal\nsignal.signal(signal.SIGTERM, signal.SIG_IGN)\n'
                     'sys.stdout.write("Content-Type: text/plain\\r\\n\\r\\nERKEN"); sys.stdout.flush()\n'
                     'os.close(1)\ntime.sleep(15)\n',
}
FILES = {
    "a/index.html": "<h1>SITE-A</h1>", "a/dosya.txt": "merhaba", "a/hata404.html": "OZEL-404",
    "a/style.css": "body{}", "a/app.js": "1;", "a/liste_acik/x.txt": "x", "a/liste_acik/y.txt": "y",
    "a/liste_kapali/z.txt": "z", "a/salt/sil.txt": "kalmali", "a/gizli.txt": "gizli",
    "a/yukle/.keep": "", "a/yukle/altdir/.keep": "", "a/bos.html": "",
    "www/pouic/toto/pouet": "POUET", "b/index.html": "SITE-B", "c/index.html": "SITE-C",
    "v1/index.html": "VHOST-1", "v2/index.html": "VHOST-2", "k/.keep": "",
    "cgi/data.txt": "GORELI-VERI", "cgi/hello.txt": CGI["hello.py"],
}


def make_fixtures():
    for rel, content in list(FILES.items()) + [("cgi/" + k, v) for k, v in CGI.items()]:
        path = os.path.join(D, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            f.write(content)
    with open(os.path.join(D, "a/img.png"), "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + bytes(range(256)))
    with open(os.path.join(D, "a/buyuk.bin"), "wb") as f:
        f.truncate(9 * 1024 * 1024)  # sunucu MAX_FILE_SIZE (10 MB) ustune 500 donuyor
    os.chmod(os.path.join(D, "a/gizli.txt"), 0)


def write_conf(name, body):
    path = os.path.join(D, name)
    with open(path, "w") as f:
        f.write(body)
    return path


def main_conf(off=0):
    d = D
    cgi = "cgi .py /usr/bin/python3;"
    return write_conf(f"eval{off}.conf", f"""
server {{
    listen 127.0.0.1:{P + off};
    server_name eval.local;
    client_max_body_size 300M;
    error_page 404 /hata404.html;
    location / {{ root {d}/a; allow_methods GET POST; index index.html; autoindex off; client_max_body_size 300M; }}
    location /liste_acik {{ root {d}/a; allow_methods GET; autoindex on; }}
    location /liste_kapali {{ root {d}/a; allow_methods GET; autoindex off; }}
    location /salt {{ root {d}/a; allow_methods GET; }}
    location /yukle {{ root {d}/a; allow_methods GET POST DELETE; upload_enable on; upload_store {d}/a/yukle; client_max_body_size 300M; }}
    location /kapouet {{ alias {d}/www; allow_methods GET; }}
    location /cgi {{ root {d}; allow_methods GET POST; {cgi} client_max_body_size 300M; }}
    location /eski {{ return 301 /; }}
}}
server {{
    listen 127.0.0.1:{P_LIMIT + off};
    server_name limit.local;
    client_max_body_size 20;
    location / {{ root {d}/k; allow_methods GET POST; upload_enable on; upload_store {d}/k; client_max_body_size 20; }}
}}
server {{
    listen 127.0.0.2:{P + off};
    server_name b.local;
    location / {{ root {d}/b; allow_methods GET; index index.html; }}
}}
server {{
    listen 127.0.0.1:{P_PLAIN + off};
    location / {{ root {d}/c; allow_methods GET; index index.html; }}
    location /cgi {{ root {d}; allow_methods GET POST; {cgi} }}
}}
server {{
    listen 127.0.0.1:{P_VHOST + off};
    server_name v1.test;
    location / {{ root {d}/v1; allow_methods GET; index index.html; }}
}}
server {{
    listen 127.0.0.1:{P_VHOST + off};
    server_name v2.test;
    location / {{ root {d}/v2; allow_methods GET; index index.html; }}
}}
""")


ALL_PORTS = (P, P_LIMIT, P_PLAIN, P_VHOST)


# ---------------------------------------------------------------- Katman 1

def test_args():
    # C-01: fazla arguman -> kullanim hatasi, cokmeden cikis
    p = subprocess.run(WRAP + [BIN, "a", "b"], capture_output=True, timeout=30 * SLOW, cwd=ROOT)
    report("C-01", p.returncode >= 0 and p.stderr.strip() != b"",
           f"./webserv a b -> cikis {p.returncode}, stderr {p.stderr[:80]!r}")
    # C-01: argumansiz -> varsayilan config (conf/default.conf, 127.0.0.1:8081)
    s = Server("")
    s.proc = subprocess.Popen(WRAP + [BIN], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL, cwd=ROOT)
    up = False
    for _ in range(int(50 * SLOW)):
        if http("GET", "/", port=8081).status == 200:
            up = True
            break
        time.sleep(0.2)
    s.stop()
    report("C-01", up, "argumansiz calisinca varsayilan config ile 8081'de 200")


def test_invalid_configs():
    unreadable = write_conf("okunamaz.conf", f"server {{ listen 127.0.0.1:{P}; }}")
    os.chmod(unreadable, 0)
    cases = sorted(glob.glob("tests/test_configs/invalid_cases/**/*.conf", recursive=True))
    cases += [os.path.join(D, "olmayan.conf"), D, unreadable]
    bad = []
    for conf in cases:
        p = subprocess.Popen(WRAP + [BIN, conf], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=ROOT)
        try:
            out, err = p.communicate(timeout=5 * SLOW)
        except subprocess.TimeoutExpired:
            p.kill()
            p.communicate()
            bad.append(f"{conf}: gecersiz config ile calismaya devam etti")
            continue
        if p.returncode < 0:
            bad.append(f"{conf}: sinyalle oldu ({signal.Signals(-p.returncode).name})")
        elif not (out + err).strip():
            bad.append(f"{conf}: hata mesaji yok")
    report("C-02", not bad, f"{len(cases)} gecersiz config cokmeden reddedildi" if not bad else "; ".join(bad))


def test_status_table():
    r = http("GET", "/dosya.txt")
    report("H-01", r.status == 200 and r.body == b"merhaba", f"GET /dosya.txt -> {r}")
    report("H-08", bool(re.match(rb"HTTP/1\.[01] 200 \S", r.head))
           and r.headers.get("content-length") == "7", f"status satiri {r.head[:40]!r}")
    cases = [
        ("C-06", "GET /yok -> 404 + ozel sayfa", http("GET", "/yok"), lambda r: r.status == 404 and b"OZEL-404" in r.body),
        ("C-09", "GET / -> index", http("GET", "/"), lambda r: r.status == 200 and b"SITE-A" in r.body),
        ("C-12", "listeleme acik -> 200 + liste", http("GET", "/liste_acik/"),
         lambda r: r.status == 200 and b"x.txt" in r.body and b"y.txt" in r.body),
        ("C-12", "listeleme kapali, index yok -> 404", http("GET", "/liste_kapali/"), lambda r: r.status == 404),
        ("H-03", "okuma izni yok -> 403", http("GET", "/gizli.txt"),
         lambda r: r.status == 403 or os.geteuid() == 0),
        ("C-11", "redirect -> 301 + Location", http("GET", "/eski"),
         lambda r: r.status == 301 and r.headers.get("location") == "/"),
        ("C-08", "/kapouet/pouic/toto/pouet -> www/pouic/toto/pouet", http("GET", "/kapouet/pouic/toto/pouet"),
         lambda r: r.status == 200 and r.body == b"POUET"),
        ("H-02", "FOO -> 501/405", http("FOO", "/"), lambda r: r.status in (501, 405)),
        ("H-02", "PUT -> 501/405", http("PUT", "/", body=b"x"), lambda r: r.status in (501, 405)),
        ("H-02", "HEAD -> 501/405", http("HEAD", "/"), lambda r: r.status in (501, 405)),
        ("H-03", "bozuk istek satiri -> 400", raw(b"MERHABA\r\n\r\n"), lambda r: r.status == 400),
        ("H-03", "HTTP/1.1 Host yok -> 400", raw(b"GET / HTTP/1.1\r\nConnection: close\r\n\r\n"),
         lambda r: r.status == 400),
        ("H-03", "Content-Length + Transfer-Encoding -> 400",
         raw(b"POST /yukle/cl.txt HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\n"
             b"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n\r\n"), lambda r: r.status == 400),
        ("H-03", "HTTP/2.0 -> 505", raw(b"GET / HTTP/2.0\r\nHost: x\r\nConnection: close\r\n\r\n"),
         lambda r: r.status == 505),
        ("H-03", "POST, Content-Length/chunked yok -> 411",
         raw(b"POST /yukle/z.txt HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"), lambda r: r.status == 411),
        ("H-03", "olmayan dosyaya DELETE -> 404", http("DELETE", "/yukle/olmayan.txt"), lambda r: r.status == 404),
        ("H-04", "text/html", http("GET", "/index.html"), lambda r: "text/html" in r.headers.get("content-type", "")),
        ("H-04", "text/css", http("GET", "/style.css"), lambda r: "text/css" in r.headers.get("content-type", "")),
        ("H-04", "javascript", http("GET", "/app.js"), lambda r: "javascript" in r.headers.get("content-type", "")),
        ("H-04", "image/png", http("GET", "/img.png"),
         lambda r: r.headers.get("content-type") == "image/png" and len(r.body) == 264),
    ]
    for rid, name, r, ok in cases:
        report(rid, ok(r), f"{name}: {r}")

    r = http("DELETE", "/salt/sil.txt")
    report("C-10", r.status == 405 and os.path.exists(f"{D}/a/salt/sil.txt"),
           f"izinsiz route'ta DELETE -> 405, dosya yerinde: {r}")
    for path in ("/../../../../etc/passwd", "/%2e%2e/%2e%2e/%2e%2e/%2e%2e/etc/passwd",
                 "/kapouet/../../../../etc/passwd", "/a%00b"):
        r = raw(f"GET {path} HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n".encode())
        report("C-08", r.status != 200 and b"root:" not in r.body, f"kok disi {path} -> {r}")


def test_upload():
    blob = os.urandom(1024 * 1024)
    r = http("POST", "/yukle/r.bin", body=blob, headers={"Content-Type": "application/octet-stream"})
    stored = open(f"{D}/a/yukle/r.bin", "rb").read() if os.path.exists(f"{D}/a/yukle/r.bin") else None
    report("C-13", r.status == 201 and stored == blob, f"ham upload -> {r}, upload_store'da ayni: {stored == blob}")
    r = http("GET", "/yukle/r.bin")
    report("H-05", r.status == 200 and r.body == blob, f"ham upload geri alindi bayt bayt: {r.status}")

    part = os.urandom(200 * 1024)
    bnd = "----evalSINIR7f3a9c"
    body = (f"--{bnd}\r\nContent-Disposition: form-data; name=\"aciklama\"\r\n\r\nmetin\r\n"
            f"--{bnd}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"m.bin\"\r\n"
            f"Content-Type: application/octet-stream\r\n\r\n").encode() + part + f"\r\n--{bnd}--\r\n".encode()
    r = http("POST", "/yukle", body=body, headers={"Content-Type": f"multipart/form-data; boundary={bnd}"})
    g = http("GET", "/yukle/m.bin")
    report("H-05", r.status == 201 and g.body == part, f"multipart upload -> {r.status}, geri alinan ayni: {g.body == part}")

    cblob = os.urandom(300 * 1024 + 17)
    r = http("POST", "/yukle/c.bin", body=cblob, chunked=True, headers={"Content-Type": "application/octet-stream"})
    g = http("GET", "/yukle/c.bin")
    report("H-07", r.status == 201 and g.body == cblob, f"chunked upload -> {r.status}, cerceve temizlendi: {g.body == cblob}")

    r = http("POST", "/salt/yeni.txt", body=b"olmamali")
    report("C-13", 400 <= (r.status or 0) < 500 and not os.path.exists(f"{D}/a/salt/yeni.txt"),
           f"upload kapali route -> {r}")

    r = http("DELETE", "/yukle/r.bin")
    g = http("GET", "/yukle/r.bin")
    report("H-06", r.status in (200, 204) and g.status == 404, f"DELETE -> {r.status}, sonra GET -> {g.status}")


def test_body_limit():
    hdr = {"Content-Type": "plain/text"}
    for n, want in ((10, False), (20, False), (21, True)):
        r = http("POST", f"/b{n}.txt", port=P_LIMIT, body=b"B" * n, headers=hdr)
        ok = (r.status == 413) if want else (r.status is not None and r.status != 413 and r.status < 500)
        report("C-07", ok, f"limit 20, govde {n} bayt -> {r}")
    r = http("POST", "/c21.txt", port=P_LIMIT, body=b"B" * 21, chunked=True, headers=hdr)
    report("C-07", r.status == 413, f"limit 20, chunked 21 bayt -> {r}")


def test_sites():
    r = http("GET", "/", ip="127.0.0.2")
    report("C-03", r.status == 200 and b"SITE-B" in r.body, f"127.0.0.2:{P} -> {r}")
    r = http("GET", "/", port=P_PLAIN)
    report("C-03", r.status == 200 and b"SITE-C" in r.body, f"127.0.0.1:{P_PLAIN} -> {r}")
    for host, want in (("v1.test", b"VHOST-1"), ("v2.test", b"VHOST-2"), ("bilinmeyen.test", b"VHOST-1")):
        r = http("GET", "/", port=P_VHOST, host=host)
        report("C-04", r.status == 200 and want in r.body, f"Host {host} -> {r}")


def test_default_error_pages():
    for name, r, code in (("404", http("GET", "/yok", port=P_PLAIN), 404),
                          ("405", http("DELETE", "/", port=P_PLAIN), 405),
                          ("400", raw(b"MERHABA\r\n\r\n", port=P_PLAIN), 400)):
        ok = r.status == code and "text/html" in r.headers.get("content-type", "") and len(r.body) > 0
        report("C-05", ok, f"error_page'siz server {name}: {r} ctype={r.headers.get('content-type')}")


def test_keepalive():
    try:
        with socket.create_connection(("127.0.0.1", P), timeout=5 * SLOW) as s:
            s.sendall(b"GET /dosya.txt HTTP/1.1\r\nHost: x\r\n\r\n")
            r1 = read_response(s)
            if "close" in r1.headers.get("connection", "").lower():
                s.settimeout(3 * SLOW)
                ok = s.recv(1) == b""
                report("H-09", r1.status == 200 and ok, "Connection: close gonderip kapatti")
            else:
                s.sendall(b"GET /dosya.txt HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
                r2 = read_response(s)
                report("H-09", r1.status == 200 and r2.status == 200, f"ayni baglantida 2. istek -> {r2}")
    except OSError as e:
        report("H-09", False, f"{type(e).__name__}: {e}")


def test_cgi():
    r = http("GET", "/cgi/hello.py")
    t = http("GET", "/cgi/hello.txt")
    report("CGI-01", r.status == 200 and r.body == b"CGI-HELLO" and b"sys.stdout" in t.body,
           f".py calisti: {r}; .txt statik: {t.status}")
    r = http("GET", "/cgi/env.py?a=1&b=2")
    report("CGI-02", b"QUERY_STRING=a=1&b=2\n" in r.body, f"GET query string: {r.status}")
    blob = os.urandom(1024 * 1024)
    want = f"len={len(blob)} md5={hashlib.md5(blob).hexdigest()}".encode()
    r = http("POST", "/cgi/body.py", body=blob, headers={"Content-Type": "application/octet-stream"})
    report("CGI-02", r.status == 200 and r.body == want, f"POST 1 MB stdin'e ulasti: {r}")
    r = http("POST", "/cgi/body.py", body=blob[:300000], chunked=True)
    want = f"len=300000 md5={hashlib.md5(blob[:300000]).hexdigest()}".encode()
    report("CGI-04", r.status == 200 and r.body == want, f"chunked POST, EOF ile bitti: {r}")

    r = http("POST", "/cgi/env.py?q=1", body=b"abc", headers={"Content-Type": "text/plain", "X-Deneme": "42"})
    env = dict(l.split("=", 1) for l in r.body.decode(errors="replace").splitlines() if "=" in l)
    expect = {"REQUEST_METHOD": "POST", "QUERY_STRING": "q=1", "CONTENT_LENGTH": "3",
              "CONTENT_TYPE": "text/plain", "SERVER_PORT": str(P), "GATEWAY_INTERFACE": "CGI/1.1",
              "HTTP_X_DENEME": "42"}
    present = ("SCRIPT_NAME", "PATH_INFO", "SERVER_PROTOCOL", "SERVER_NAME", "REMOTE_ADDR", "HTTP_HOST")
    wrong = [f"{k}={env.get(k)!r} (beklenen {v!r})" for k, v in expect.items() if env.get(k) != v]
    wrong += [f"{k} yok" for k in present if k not in env]
    report("CGI-03", r.status == 200 and not wrong, "ortam degiskenleri tam" if not wrong else "; ".join(wrong))

    r = http("GET", "/cgi/stream.py", timeout=30)
    report("CGI-05", r.status == 200 and len(r.body) == 80 * 65536,
           f"Content-Length'siz CGI ciktisi EOF'a kadar: {len(r.body)} / {80 * 65536} bayt")
    r = http("GET", "/cgi/cwd.py")
    report("CGI-06", r.status == 200 and b"GORELI-VERI" in r.body, f"goreli yol: {r}")
    for name in ("syntax.py", "exit1.py", "badheader.py"):
        r = http("GET", f"/cgi/{name}")
        report("CGI-07", r.status in (500, 502), f"{name} -> {r}")
    r = http("GET", "/cgi/yok.py")
    report("CGI-07", r.status == 404, f"olmayan script -> {r}")


def test_cgi_early_close(srv):
    # stdout'u kapatip 15 sn daha calisan CGI: EOF'tan sonraki waitpid sunucuyu bekletmemeli
    res = {}

    def run():
        t0 = time.time()
        res["cgi"] = (http("GET", "/cgi/earlyclose.py", timeout=30), time.time() - t0)

    t = threading.Thread(target=run)
    t.start()
    time.sleep(0.5 * SLOW)
    t0 = time.time()
    g = http("GET", "/", timeout=30)
    dt = time.time() - t0
    t.join()
    r, cgi_dt = res["cgi"]
    report("CGI-07", r.status == 200 and r.body == b"ERKEN" and cgi_dt < 2 * SLOW,
           f"stdout'u erken kapatan CGI -> {r}, {cgi_dt:.2f} sn")
    report("CGI-07", g.status == 200 and dt < SLOW, f"o CGI calisirken GET / {dt:.2f} sn")
    kids = srv.children()
    deadline = time.time() + 3 * SLOW
    while kids and time.time() < deadline:
        time.sleep(0.2)
        kids = srv.children()
    report("CGI-07", not kids, f"stdout'u erken kapatan CGI'dan artik/zombi surec yok: {kids}")


def test_nonblocking(srv):
    if WRAP:
        return
    t = threading.Thread(target=lambda: http("GET", "/cgi/slow.py"))
    t.start()
    idle = socket.create_connection(("127.0.0.1", P))
    time.sleep(1)
    blocking = []
    for fd in os.listdir(f"/proc/{srv.proc.pid}/fd"):
        try:
            link = os.readlink(f"/proc/{srv.proc.pid}/fd/{fd}")
            flags = int(re.search(r"flags:\s*(\d+)", open(f"/proc/{srv.proc.pid}/fdinfo/{fd}").read()).group(1), 8)
        except (OSError, AttributeError):
            continue
        if int(fd) > 2 and link.startswith(("socket:", "pipe:")) and not flags & 0o4000:
            blocking.append(f"fd {fd} {link}")
    t.join()
    idle.close()
    report("IO-08", not blocking, "dinleme/istemci soketleri ve CGI pipe'lari non-blocking"
           if not blocking else f"blocking: {blocking}")


ABUSE = [
    ("rastgele bayt", lambda: os.urandom(10000)),
    ("100 KB URI", lambda: b"GET /" + b"a" * 100000 + b" HTTP/1.1\r\nHost: x\r\n\r\n"),
    ("100 KB header", lambda: b"GET / HTTP/1.1\r\nHost: x\r\nX-Buyuk: " + b"a" * 100000 + b"\r\n\r\n"),
    ("10000 header", lambda: b"GET / HTTP/1.1\r\nHost: x\r\n" + b"X-A: b\r\n" * 10000 + b"\r\n"),
    ("versiyonsuz", lambda: b"GET /\r\n\r\n"),
    ("bos satirlar", lambda: b"\r\n\r\n\r\n\r\n"),
    ("CL -1", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nContent-Length: -1\r\n\r\nabc"),
    ("CL dev", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nContent-Length: 99999999999999999999\r\n\r\nabc"),
    ("CL abc", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\nabc"),
    ("iki CL", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\nContent-Length: 5\r\n\r\nabcde"),
    ("chunk zz", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\nab\r\n0\r\n\r\n"),
    ("chunk -5", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n-5\r\nab\r\n0\r\n\r\n"),
    ("chunk tasma", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
                            b"FFFFFFFFFFFFFFFFFFFF\r\nab\r\n0\r\n\r\n"),
    ("bitmeyen chunked", lambda: b"POST /yukle/a HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nab"),
    ("dizin DELETE", lambda: b"DELETE /yukle/altdir/ HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"),
]


def test_abuse(srv):
    base = srv.fds()
    for name, make in ABUSE:
        r = raw(make(), timeout=3)
        if not srv.alive():
            report("R-01", False, f"'{name}' sonrasi sunucu oldu (cikis {srv.proc.returncode})")
            srv.stop()
            if not srv.start(ALL_PORTS):
                return
            base = srv.fds()
        else:
            report("R-01", True, f"'{name}' -> {r.status or r.error}")

    for _ in range(20):  # istek yarida kesilir
        with socket.create_connection(("127.0.0.1", P), timeout=10 * SLOW) as s:
            s.sendall(b"GET / HTTP/1.1\r\nHo")
    r = http("GET", "/cgi/hello.py", port=P_PLAIN)
    time.sleep(0.5)
    code = srv.proc.poll()
    report("R-01", code is None, f"server_name'siz server'da CGI -> {r.status or r.error}, "
           + ("surec ayakta" if code is None else f"surec oldu (cikis {code})"))
    if code is not None:
        srv.stop()
        srv.start(ALL_PORTS)
        base = srv.fds()

    first = b""
    for i in range(30):  # yanit yarida kesilir; yarisi RST ile (SIGPIPE/EPIPE)
        s = socket.create_connection(("127.0.0.1", P), timeout=10 * SLOW)
        s.sendall(b"GET /buyuk.bin HTTP/1.1\r\nHost: x\r\n\r\n")
        got = s.recv(4096)
        first = first or got
        if i % 2:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b"\x01\x00\x00\x00\x00\x00\x00\x00")
        s.close()
    after = http("GET", "/")
    report("R-01", srv.proc.poll() is None and first.startswith(b"HTTP/1.1 200"),
           f"yaniti yarida kesilen 30 baglanti sonrasi surec ayakta (buyuk dosya yaniti {first[:15]!r})")
    report("R-02", after.status == 200, f"yaniti yarida kesilen 30 baglanti sonrasi GET / -> {after}")

    for i in range(30):  # istek gonderilir, yanit beklenmeden kapatilir
        s = socket.create_connection(("127.0.0.1", P), timeout=10 * SLOW)
        s.sendall(b"GET /buyuk.bin HTTP/1.1\r\nHost: x\r\n\r\n")
        s.close()
    after = http("GET", "/")
    report("R-01", srv.proc.poll() is None, "istek atip hemen kapanan 30 baglanti sonrasi surec ayakta")
    report("R-02", after.status == 200, f"istek atip hemen kapanan 30 baglanti sonrasi GET / -> {after}")
    now = srv.fds()
    for _ in range(int(20 * SLOW)):
        if now <= base:
            break
        time.sleep(0.5)
        now = srv.fds()
    report("IO-07", now <= base, f"acik fd: once {base}, sonra {now}")



def test_second_instance(srv):
    conf = write_conf("ikinci.conf", f"server {{ listen 127.0.0.1:{P}; location / {{ root {D}/b; allow_methods GET; index index.html; }} }}\n"
                                     f"server {{ listen 127.0.0.1:18085; location / {{ root {D}/b; allow_methods GET; index index.html; }} }}\n")
    p = subprocess.Popen(WRAP + [BIN, conf], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=ROOT)
    try:
        out, _ = p.communicate(timeout=3 * SLOW)
        how = f"cikti (kod {p.returncode}): {out[-120:]!r}"
        ok = p.returncode >= 0 and out.strip() != b""
    except subprocess.TimeoutExpired:
        other = http("GET", "/", port=18085)
        p.send_signal(signal.SIGINT)
        try:
            p.communicate(timeout=15 * SLOW)
        except subprocess.TimeoutExpired:  # beklemeye devam etseydik CI is zaman asimina kadar takilirdi
            p.kill()
            p.communicate()
        report("R-01", p.returncode >= 0, f"ortak portlu ikinci surec SIGINT ile cikis kodu {p.returncode}")
        how = f"calismaya devam etti, 18085 -> {other.status}"
        ok = other.status == 200
    report("P-03", ok and srv.alive(), f"ortak portlu ikinci surec {how}; ilk surec ayakta: {srv.alive()}")


# ---------------------------------------------------------------- --heavy

def test_timeouts(srv):
    limit = IDLE + 5
    out = {}

    def wait_close(name, payload, expect=None):
        t0 = time.time()
        try:
            with socket.create_connection(("127.0.0.1", P), timeout=limit + 5) as s:
                if payload:
                    s.sendall(payload)
                data = b""
                while True:
                    d = s.recv(65536)
                    if not d:
                        break
                    data += d
            m = re.match(rb"HTTP/1\.[01] (\d{3})", data)
            out[name] = (time.time() - t0, int(m.group(1)) if m else None)
        except OSError as e:
            out[name] = (time.time() - t0, str(e))

    def not_reading():
        with socket.socket() as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)  # connect'ten once: pencere kucuk kalsin
            s.connect(("127.0.0.1", P))
            s.sendall(b"GET /buyuk.bin HTTP/1.1\r\nHost: x\r\n\r\n")
            time.sleep(limit)
            s.settimeout(10)
            data = b""
            try:
                while True:
                    d = s.recv(1 << 20)
                    if not d:
                        break
                    data += d
            except OSError:
                pass
            out["okumayan"] = (data[:15], len(data), os.path.getsize(f"{D}/a/buyuk.bin"))

    def cgi_hang():
        t0 = time.time()
        r = http("GET", "/cgi/hang.py", timeout=limit + 5)
        out["cgi"] = (time.time() - t0, r.status)

    jobs = [threading.Thread(target=wait_close, args=("bos", b"")),
            threading.Thread(target=wait_close, args=("yarim header", b"GET / HTTP/1.1\r\nHost: x\r\n")),
            threading.Thread(target=wait_close, args=("yarim govde",
                             b"POST /yukle/y.txt HTTP/1.1\r\nHost: x\r\nContent-Length: 100\r\n\r\n0123456789")),
            threading.Thread(target=not_reading), threading.Thread(target=cgi_hang)]
    for j in jobs:
        j.start()
    time.sleep(1)
    t0 = time.time()
    quick = http("GET", "/dosya.txt")
    report("CGI-07", quick.status == 200 and time.time() - t0 < 1,
           f"sonsuz CGI surerken baska istek {time.time() - t0:.2f} sn")
    for j in jobs:
        j.join()
    for name in ("bos", "yarim header", "yarim govde"):
        dt, code = out[name]
        report("R-02", dt <= limit and code in (None, 408), f"{name}: {dt:.1f} sn sonra kapandi, status {code}")
    first, got, total = out["okumayan"]
    report("R-02", first.startswith(b"HTTP/1.1 200") and got < total,
           f"okumayan istemci: yanit {first!r}, {got}/{total} bayt sonra kapandi")
    dt, code = out["cgi"]
    report("CGI-07", code == 504 and dt <= limit, f"sonsuz CGI -> {code}, {dt:.1f} sn")
    time.sleep(2)
    kids = srv.children()
    report("CGI-07", not kids, f"artik/zombi CGI sureci yok: {kids}")


def timed_get():
    t0 = time.time()
    r = http("GET", "/dosya.txt")
    return r.status, time.time() - t0


def test_disk_stall():
    stop = threading.Event()
    status = {}

    def slow_reader():
        with socket.create_connection(("127.0.0.1", P)) as s:
            s.sendall(b"GET /buyuk.bin HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
            status["get"] = s.recv(15)
            while not stop.is_set() and s.recv(65536):
                time.sleep(0.05)

    t = threading.Thread(target=slow_reader)
    t.start()
    time.sleep(1)
    samples = [timed_get() for _ in range(5)]
    stop.set()
    t.join()
    report("IO-09", status.get("get", b"").startswith(b"HTTP/1.1 200") and all(c == 200 and dt < 1 for c, dt in samples),
           f"9 MB yavas indirme ({status.get('get')!r}) surerken kucuk GET: " + ", ".join(f"{dt:.2f}s" for _, dt in samples))

    def upload():
        status["post"] = http("POST", "/yukle/buyuk-up.bin", body=b"\0" * (50 * 1024 * 1024), timeout=120).status

    t = threading.Thread(target=upload)
    t.start()
    time.sleep(0.5)
    samples = [timed_get() for _ in range(5)]
    t.join()
    report("IO-09", status.get("post") == 201 and all(c == 200 and dt < 1 for c, dt in samples),
           f"50 MB upload ({status.get('post')}) surerken kucuk GET: " + ", ".join(f"{dt:.2f}s" for _, dt in samples))


def test_cgi_big():
    blob = os.urandom(10 * 1024 * 1024)
    res = {}
    t1 = threading.Thread(target=lambda: res.update(post=http("POST", "/cgi/body.py", body=blob, timeout=60)))
    t2 = threading.Thread(target=lambda: res.update(get=http("GET", "/cgi/big.py", timeout=60)))
    t1.start()
    t2.start()
    time.sleep(0.3)
    samples = [timed_get() for _ in range(3)]
    t1.join()
    t2.join()
    want = f"len={len(blob)} md5={hashlib.md5(blob).hexdigest()}".encode()
    report("CGI-08", res["post"].body == want, f"10 MB POST CGI'a butun ulasti: {res['post']}")
    report("CGI-08", res["get"].status == 200 and len(res["get"].body) == 160 * 65536,
           f"10 MB CGI ciktisi butun: {len(res['get'].body)}")
    report("CGI-08", all(c == 200 and dt < 1 for c, dt in samples),
           "CGI'lar surerken kucuk GET: " + ", ".join(f"{dt:.2f}s" for _, dt in samples))


def test_limits():
    # R-01: fd limiti dolunca accept basarisiz olur, sunucu ayakta kalmali
    srv = Server(main_conf(100), wrap=["prlimit", "--nofile=64:64"])
    if srv.start((P + 100,)):
        socks = []
        for _ in range(150):
            try:
                socks.append(socket.create_connection(("127.0.0.1", P + 100), timeout=2))
            except OSError:
                break
        time.sleep(1)
        for s in socks:
            s.close()
        time.sleep(1)
        report("R-01", srv.alive(P + 100), f"fd limiti 64, {len(socks)} baglanti acildi; sonra sunucu ayakta")
        srv.stop()
    # R-01: bellek siniri (bad_alloc yakalanmali)
    srv = Server(main_conf(200), wrap=["prlimit", "--as=268435456"])
    if srv.start((P + 200,)):
        jobs = [threading.Thread(target=http, args=("POST", f"/yukle/oom{i}.bin"),
                                 kwargs={"port": P + 200, "body": b"\0" * (100 * 1024 * 1024), "timeout": 60})
                for i in range(4)]
        for j in jobs:
            j.start()
        for j in jobs:
            j.join()
        report("R-01", srv.alive(P + 200), f"256 MB bellek siniri, 4x100 MB POST; sunucu ayakta (cikis {srv.proc.returncode})")
        srv.stop()


# ---------------------------------------------------------------- --siege

def test_siege(srv):
    if not shutil.which("siege"):
        report("S-01", False, "siege kurulu degil (BELIRSIZ)")
        return
    url = f"http://127.0.0.1:{P}/bos.html"
    base_fds = srv.fds()
    rss = []
    for i in range(3):
        p = subprocess.run(["siege", "-b", "-c", "50", "-t", "1M", url], capture_output=True, text=True)
        m = re.findall(r"[Aa]vailability\"?:\s*([\d.]+)", p.stdout + p.stderr)
        avail = float(m[-1]) if m else 0.0
        report("S-01", avail > 99.5, f"kosu {i + 1}: siege -b -c 50 -t 1M availability {avail}%")
        rss.append(int(open(f"/proc/{srv.proc.pid}/status").read().split("VmRSS:")[1].split()[0]))
    report("S-02", rss[2] - rss[1] <= 1024, f"RSS (KB) kosu sonlarinda: {rss}")
    p = subprocess.run(["siege", "-b", "-c", "100", "-r", "200", url], capture_output=True, text=True)
    m = re.findall(r"[Aa]vailability\"?:\s*([\d.]+)", p.stdout + p.stderr)
    report("S-01", m and float(m[-1]) > 99.5, f"siege -b -c 100 -r 200 availability {m[-1] if m else '?'}%")
    time.sleep(IDLE + 5)
    ss = subprocess.run(["ss", "-Htan", f"( sport = :{P} )"], capture_output=True, text=True).stdout
    hanging = [l for l in ss.splitlines() if l.startswith(("ESTAB", "CLOSE-WAIT"))]
    report("S-03", not hanging and srv.fds() <= base_fds,
           f"asili baglanti {len(hanging)}, fd {base_fds} -> {srv.fds()}")
    report("S-04", srv.alive(), "siege sonrasi yeniden baslatmadan 200 (10 dk'lik kosu CI'da 4 dk'ya kisaltildi)")


# ---------------------------------------------------------------- --valgrind

def valgrind_report():
    project = {os.path.basename(f) for f in glob.glob("src/**/*.cpp", recursive=True) + ["main.cpp"]}
    logs = sorted(glob.glob(f"{D}/vg-*.log"))
    bad, crashed = [], 0
    for log in logs:
        text = open(log).read()
        cmd = re.search(r"Command: (.*)", text)
        cmd = cmd.group(1) if cmd else log
        if "Process terminating with default action of signal" in text:
            crashed += 1  # cokmus surecin sizinti raporu anlamsiz; cokme R-01'de raporlandi
            continue
        for kind in ("definitely lost", "indirectly lost", "possibly lost"):
            m = re.search(rf"{kind}: ([\d,]+) bytes", text)
            if m and m.group(1) != "0":
                bad.append(f"{cmd}: {kind} {m.group(1)} bayt")
        for rec in re.split(r"\n==\d+== \n", text):
            if "still reachable in loss record" in rec and any(f"({f}:" in rec for f in project):
                frame = next(l for l in rec.splitlines() if any(f"({f}:" in l for f in project))
                bad.append(f"{cmd}: proje kodundan still reachable, ilk kare {frame.split(': ', 1)[-1]}")
                break
    note = f" ({crashed} cokmus surec logu atlandi)" if crashed else ""
    report("S-05", logs and not bad, f"{len(logs)} valgrind logu temiz{note}" if not bad
           else "; ".join(sorted(set(bad))[:10]) + note)


# ---------------------------------------------------------------- --bonus

PHP_CGI = "/usr/bin/php-cgi"
P_BONUS = P + 300
BONUS_CGI = {
    "hello.py": CGI["hello.py"],
    "hello.php": '<?php header("Content-Type: text/plain"); echo "PHP-HELLO";\n',
    "env.php": '<?php header("Content-Type: text/plain");\n'
               'foreach (["REQUEST_METHOD", "QUERY_STRING", "CONTENT_LENGTH", "CONTENT_TYPE", "SERVER_PORT",\n'
               '          "GATEWAY_INTERFACE", "HTTP_X_DENEME"] as $k) echo $k . "=" . getenv($k) . "\\n";\n',
    "body.php": '<?php header("Content-Type: text/plain"); $b = file_get_contents("php://input");\n'
                'echo "len=" . strlen($b) . " md5=" . md5($b);\n',
    "stream.php": '<?php header("Content-Type: application/octet-stream");\n'
                  'for ($i = 0; $i < 80; $i++) { echo str_repeat("x", 65536); flush(); }\n',
    "cwd.php": '<?php header("Content-Type: text/plain"); echo file_get_contents("data.txt");\n',
    "data.txt": "GORELI-VERI",
    "syntax.php": "<?php def (\n",
    "throw.php": '<?php throw new Exception("hata");\n',
    "session.py": 'import os,sys\n'
                  'c = dict(p.strip().split("=", 1) for p in os.environ.get("HTTP_COOKIE", "").split(";") if "=" in p)\n'
                  'sid = c.get("sid", "")\n'
                  'if not sid.isalnum(): sid = os.urandom(8).hex()\n'
                  'os.makedirs("oturum", exist_ok=True)\n'
                  'f = os.path.join("oturum", sid)\n'
                  'n = int(open(f).read()) + 1 if os.path.exists(f) else 1\n'
                  'open(f, "w").write(str(n))\n'
                  'sys.stdout.write("Content-Type: text/plain\\r\\nSet-Cookie: sid=%s\\r\\n\\r\\nsid=%s count=%d" % (sid, sid, n))\n',
    "twocookie.py": 'import sys\nsys.stdout.write("Content-Type: text/plain\\r\\nSet-Cookie: a=1\\r\\nSet-Cookie: b=2\\r\\n\\r\\nOK")\n',
}


def bonus_conf():
    pairs = f"cgi .py /usr/bin/python3; cgi .php {PHP_CGI};"
    reverse = f"cgi .php {PHP_CGI}; cgi .py /usr/bin/python3;"
    return write_conf("bonus.conf", f"""
server {{
    listen 127.0.0.1:{P_BONUS};
    location / {{ root {D}/a; allow_methods GET; index index.html; }}
    location /bcgi {{ root {D}; allow_methods GET POST; {pairs} client_max_body_size 300M; }}
    location /ters {{ root {D}; allow_methods GET POST; {reverse} }}
    location /goreli {{ root {os.path.relpath(D, ROOT)}; allow_methods GET POST; {pairs} }}
}}
""")


def test_bonus():
    for sub in ("bcgi", "ters", "goreli"):
        for name, content in BONUS_CGI.items():
            path = os.path.join(D, sub, name)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w") as f:
                f.write(content)
    srv = Server(bonus_conf())
    if not srv.start((P_BONUS,)):
        report("B-02", False, f"bonus sunucusu baslamadi; log: {open(srv.log.name).read()[-500:]}")
        return
    try:
        test_bonus_cookie()
        test_bonus_cgi()
    finally:
        srv.stop()


def test_bonus_cookie():
    example = [f for f in glob.glob("**/*", recursive=True)
               if os.path.isfile(f) and not f.startswith(("tests/", "docs/", ".git/")) and f != "EVAL_RULES.md"
               and f.endswith((".py", ".php", ".html", ".pl", ".sh"))
               and re.search(r"set-cookie|setcookie|http_cookie|session_start", open(f, errors="ignore").read(), re.I)]
    report("B-01", example, f"repoda cookie/session ornegi: {example[:5]}" if example
           else "repoda cookie/session ornegi yok (S: \"provide simple examples\")")

    r1 = http("GET", "/bcgi/session.py", port=P_BONUS)
    cookie = r1.headers.get("set-cookie", "")
    sid = cookie.split(";")[0]
    report("B-01", r1.status == 200 and sid.startswith("sid=") and r1.body.endswith(b"count=1"),
           f"ilk istekte Set-Cookie: {cookie!r}, {r1}")
    r2 = http("GET", "/bcgi/session.py", port=P_BONUS, headers={"Cookie": sid})
    report("B-01", r2.status == 200 and r2.body == f"{sid} count=2".encode(),
           f"ayni cookie ile ikinci istek oturumu koruyor: {r2}")
    r3 = http("GET", "/bcgi/session.py", port=P_BONUS)
    other = r3.headers.get("set-cookie", "").split(";")[0]
    report("B-01", r3.body.endswith(b"count=1") and other.startswith("sid=") and other != sid,
           f"farkli istemci farkli oturum: {sid} / {other}")
    r = http("GET", "/bcgi/twocookie.py", port=P_BONUS)
    got = re.findall(rb"(?im)^set-cookie:\s*(\S+)", r.head)
    report("B-01", got == [b"a=1", b"b=2"], f"CGI'in iki Set-Cookie header'i da istemciye ulasti: {got}")


def test_bonus_cgi():
    if not os.access(PHP_CGI, os.X_OK):
        report("B-02", False, f"{PHP_CGI} yok, ikinci CGI turu denenemedi")
        return
    r = http("GET", "/bcgi/hello.php", port=P_BONUS)
    p = http("GET", "/bcgi/hello.py", port=P_BONUS)
    report("B-02", r.status == 200 and r.body == b"PHP-HELLO" and p.body == b"CGI-HELLO",
           f"ayni location'da .php ve .py: {r} / {p}")
    r = http("GET", "/bcgi/env.php?a=1&b=2", port=P_BONUS)
    report("B-02", b"QUERY_STRING=a=1&b=2\n" in r.body, f"CGI-02 php GET query string: {r}")
    blob = os.urandom(1024 * 1024)
    want = f"len={len(blob)} md5={hashlib.md5(blob).hexdigest()}".encode()
    r = http("POST", "/bcgi/body.php", port=P_BONUS, body=blob, headers={"Content-Type": "application/octet-stream"})
    report("B-02", r.status == 200 and r.body == want, f"CGI-02 php POST 1 MB stdin'e ulasti: {r}")
    r = http("POST", "/bcgi/body.php", port=P_BONUS, body=blob[:300000], chunked=True)
    want = f"len=300000 md5={hashlib.md5(blob[:300000]).hexdigest()}".encode()
    report("B-02", r.status == 200 and r.body == want, f"CGI-04 php chunked POST: {r}")
    r = http("POST", "/bcgi/env.php?q=1", port=P_BONUS, body=b"abc",
             headers={"Content-Type": "text/plain", "X-Deneme": "42"})
    env = dict(l.split("=", 1) for l in r.body.decode(errors="replace").splitlines() if "=" in l)
    expect = {"REQUEST_METHOD": "POST", "QUERY_STRING": "q=1", "CONTENT_LENGTH": "3", "CONTENT_TYPE": "text/plain",
              "SERVER_PORT": str(P_BONUS), "GATEWAY_INTERFACE": "CGI/1.1", "HTTP_X_DENEME": "42"}
    wrong = [f"{k}={env.get(k)!r} (beklenen {v!r})" for k, v in expect.items() if env.get(k) != v]
    report("B-02", r.status == 200 and not wrong, "CGI-03 php ortam degiskenleri tam" if not wrong else "; ".join(wrong))
    r = http("GET", "/bcgi/stream.php", port=P_BONUS, timeout=30)
    report("B-02", r.status == 200 and len(r.body) == 80 * 65536,
           f"CGI-05 php Content-Length'siz cikti: {len(r.body)} / {80 * 65536} bayt")
    r = http("GET", "/bcgi/cwd.php", port=P_BONUS)
    report("B-02", r.status == 200 and r.body == b"GORELI-VERI", f"CGI-06 php goreli yol: {r}")
    for name in ("syntax.php", "throw.php"):
        r = http("GET", f"/bcgi/{name}", port=P_BONUS)
        report("B-02", r.status in (500, 502), f"CGI-07 {name} -> {r}")
    r = http("GET", "/bcgi/yok.php", port=P_BONUS)
    report("B-02", r.status == 404, f"CGI-07 olmayan php -> {r}")

    r = http("GET", "/ters/hello.php", port=P_BONUS)
    p = http("GET", "/ters/hello.py", port=P_BONUS)
    report("B-02", r.body == b"PHP-HELLO" and p.body == b"CGI-HELLO",
           f"cgi satirlarinin sirasi ters olunca eslesme dogru: .php {r} / .py {p}")
    r = http("GET", "/goreli/hello.php", port=P_BONUS)
    p = http("GET", "/goreli/hello.py", port=P_BONUS)
    report("B-02", r.body == b"PHP-HELLO" and p.body == b"CGI-HELLO", f"goreli root ile .php {r} / .py {p}")


# ---------------------------------------------------------------- main

def main():
    make_fixtures()
    srv = Server(main_conf())
    try:
        if MODE in ("", "--valgrind"):
            test_args()
            test_invalid_configs()
        if not srv.start(ALL_PORTS):
            report("R-01", False, f"sunucu baslamadi; log: {open(srv.log.name).read()[-500:]}")
            return 1
        if MODE in ("", "--valgrind"):
            test_status_table()
            test_upload()
            test_body_limit()
            test_sites()
            test_default_error_pages()
            test_keepalive()
            test_cgi()
            test_cgi_early_close(srv)
            test_nonblocking(srv)
            test_second_instance(srv)
            test_abuse(srv)
        elif MODE == "--heavy":
            test_timeouts(srv)
            test_disk_stall()
            test_cgi_big()
            test_limits()
        elif MODE == "--siege":
            test_siege(srv)
        elif MODE == "--bonus":
            test_bonus()
        code = srv.stop()
        report("R-01", code is not None and code >= 0, f"SIGINT ile cikis kodu {code}")
        if MODE == "--valgrind":
            valgrind_report()
    finally:
        if srv.proc and srv.proc.poll() is None:
            srv.proc.kill()
        log_tail = ""
        for log in sorted(glob.glob(f"{D}/server-*.log")):
            log_tail = open(log, errors="replace").read()[-1500:]
        if os.environ.get("GITHUB_ACTIONS") and log_tail:
            print(f"\n--- son sunucu logu ---\n{log_tail}")
        os.chmod(f"{D}/a/gizli.txt", 0o644)
        shutil.rmtree(D, ignore_errors=True)
    fails = sorted({r for r, ok in results if ok is False})
    print(f"\n{len(results)} kontrol, {sum(1 for _, ok in results if ok is False)} FAIL: {fails}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
