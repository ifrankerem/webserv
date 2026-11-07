#!/usr/bin/env python3
import html
import os
import shutil
import subprocess
import sys
import tempfile
import time
from urllib.parse import parse_qs

EXAMPLES = {
    "merhaba": ("c", '#include <stdio.h>\n\nint main(void)\n{\n    printf("Merhaba, webserv!\\n");\n    return 0;\n}\n'),
    "girdi": ("cpp", '#include <iostream>\n#include <string>\n\nint main()\n{\n    std::string name;\n'
                     '    std::getline(std::cin, name);\n    std::cout << "Merhaba, " << name << "!" << std::endl;\n'
                     '    return 0;\n}\n'),
    "hata": ("c", '#include <stdio.h>\n\nint main(void)\n{\n    printf("noktali virgul yok")\n    return 0;\n}\n'),
    "sonsuz": ("c", "int main(void)\n{\n    while (1)\n        ;\n    return 0;\n}\n"),
}
RUN_TIMEOUT = 5
OUTPUT_LIMIT = 20000
ENV = {"PATH": "/usr/bin:/bin"}

method = os.environ.get("REQUEST_METHOD", "GET")
query = parse_qs(os.environ.get("QUERY_STRING", ""))
form = parse_qs(sys.stdin.buffer.read().decode("utf-8", "replace"), keep_blank_values=True) if method == "POST" else {}

example = query.get("ornek", ["merhaba"])[0]
lang, code = EXAMPLES.get(example, EXAMPLES["merhaba"])
lang = form.get("dil", [lang])[0]
code = form.get("kod", [code])[0]
stdin = form.get("girdi", [""])[0]


def clip(text):
    return text if len(text) <= OUTPUT_LIMIT else text[:OUTPUT_LIMIT] + "\n... (cikti kesildi)"


report = ""
if method == "POST":
    work = tempfile.mkdtemp(prefix="derleyici_")
    try:
        source = os.path.join(work, "main.cpp" if lang == "cpp" else "main.c")
        binary = os.path.join(work, "prog")
        with open(source, "w") as f:
            f.write(code)
        compiler = ["g++"] if lang == "cpp" else ["gcc"]
        t0 = time.time()
        build = subprocess.run(compiler + ["-Wall", "-Wextra", "-o", binary, source],
                               capture_output=True, text=True, env=ENV, timeout=30)
        report += "$ %s -Wall -Wextra main.%s  (%.2f sn)\n" % (compiler[0], lang, time.time() - t0)
        report += build.stderr.replace(work + "/", "")
        if build.returncode != 0:
            report += "\nDerleme basarisiz (cikis kodu %d)." % build.returncode
        else:
            t0 = time.time()
            try:
                run = subprocess.run([binary], input=stdin, capture_output=True, text=True,
                                     env=ENV, cwd=work, timeout=RUN_TIMEOUT)
                report += "\n$ ./prog  (%.2f sn)\n%s%s" % (time.time() - t0, run.stdout, run.stderr)
                report += "\n[cikis kodu %d]" % run.returncode
            except subprocess.TimeoutExpired:
                report += "\n$ ./prog\nProgram %d saniyede bitmedi ve durduruldu." % RUN_TIMEOUT
    finally:
        shutil.rmtree(work, ignore_errors=True)


def selected(value):
    return " selected" if value == lang else ""


print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Online C/C++ derleyici</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Online C/C++ derleyici</header>
    <main>
        <section>
            <h2>Online C/C++ derleyici</h2>
            <p class="muted">Kod POST body ile bu CGI script'ine gidiyor; script gcc/g++ ile derleyip çalıştırıyor.
            Örnekler: <a href="?ornek=merhaba">merhaba</a> · <a href="?ornek=girdi">stdin</a> ·
            <a href="?ornek=hata">derleme hatası</a> · <a href="?ornek=sonsuz">sonsuz döngü</a></p>
            <form method="POST" action="/derleyici.py" class="stack">
                <select name="dil">
                    <option value="c"%s>C (gcc)</option>
                    <option value="cpp"%s>C++ (g++)</option>
                </select>
                <textarea name="kod" rows="14" spellcheck="false">%s</textarea>
                <textarea name="girdi" rows="2" placeholder="Programın stdin girdisi (isteğe bağlı)">%s</textarea>
                <button>Derle ve çalıştır</button>
            </form>
        </section>""" % (selected("c"), selected("cpp"), html.escape(code), html.escape(stdin)))
if report:
    print("        <section><h2>Sonuç</h2><pre>%s</pre></section>" % html.escape(clip(report)))
print("    </main>\n</body>\n</html>")
