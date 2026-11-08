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
    "hello": ("c", '#include <stdio.h>\n\nint main(void)\n{\n    printf("Hello, webserv!\\n");\n    return 0;\n}\n'),
    "input": ("cpp", '#include <iostream>\n#include <string>\n\nint main()\n{\n    std::string name;\n'
                     '    std::getline(std::cin, name);\n    std::cout << "Hello, " << name << "!" << std::endl;\n'
                     '    return 0;\n}\n'),
    "error": ("c", '#include <stdio.h>\n\nint main(void)\n{\n    printf("missing semicolon")\n    return 0;\n}\n'),
    "infinite": ("c", "int main(void)\n{\n    while (1)\n        ;\n    return 0;\n}\n"),
}
RUN_TIMEOUT = 5
OUTPUT_LIMIT = 20000
ENV = {"PATH": "/usr/bin:/bin"}

method = os.environ.get("REQUEST_METHOD", "GET")
query = parse_qs(os.environ.get("QUERY_STRING", ""))
form = parse_qs(sys.stdin.buffer.read().decode("utf-8", "replace"), keep_blank_values=True) if method == "POST" else {}

example = query.get("example", ["hello"])[0]
lang, code = EXAMPLES.get(example, EXAMPLES["hello"])
lang = form.get("lang", [lang])[0]
code = form.get("code", [code])[0]
stdin = form.get("input", [""])[0]


def clip(text):
    return text if len(text) <= OUTPUT_LIMIT else text[:OUTPUT_LIMIT] + "\n... (output truncated)"


report = ""
if method == "POST":
    work = tempfile.mkdtemp(prefix="compiler_")
    try:
        source = os.path.join(work, "main.cpp" if lang == "cpp" else "main.c")
        binary = os.path.join(work, "prog")
        with open(source, "w") as f:
            f.write(code)
        compiler = ["g++"] if lang == "cpp" else ["gcc"]
        t0 = time.time()
        build = subprocess.run(compiler + ["-Wall", "-Wextra", "-o", binary, source],
                               capture_output=True, text=True, env=ENV, timeout=30)
        report += "$ %s -Wall -Wextra main.%s  (%.2f s)\n" % (compiler[0], lang, time.time() - t0)
        report += build.stderr.replace(work + "/", "")
        if build.returncode != 0:
            report += "\nCompilation failed (exit code %d)." % build.returncode
        else:
            t0 = time.time()
            try:
                run = subprocess.run([binary], input=stdin, capture_output=True, text=True,
                                     env=ENV, cwd=work, timeout=RUN_TIMEOUT)
                report += "\n$ ./prog  (%.2f s)\n%s%s" % (time.time() - t0, run.stdout, run.stderr)
                report += "\n[exit code %d]" % run.returncode
            except subprocess.TimeoutExpired:
                report += "\n$ ./prog\nThe program did not finish in %d seconds and was stopped." % RUN_TIMEOUT
    finally:
        shutil.rmtree(work, ignore_errors=True)


def selected(value):
    return " selected" if value == lang else ""


print("Content-Type: text/html; charset=UTF-8\r\n\r\n", end="")
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Online C/C++ compiler</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <header><a href="/">webserv demo</a> / Online C/C++ compiler</header>
    <main>
        <section>
            <h2>Online C/C++ compiler</h2>
            <p class="muted">The code goes to this CGI script in the POST body; the script compiles and runs it with gcc/g++.
            Examples: <a href="?example=hello">hello</a> · <a href="?example=input">stdin</a> ·
            <a href="?example=error">compile error</a> · <a href="?example=infinite">infinite loop</a></p>
            <form method="POST" action="/compiler.py" class="stack">
                <select name="lang">
                    <option value="c"%s>C (gcc)</option>
                    <option value="cpp"%s>C++ (g++)</option>
                </select>
                <textarea name="code" rows="14" spellcheck="false">%s</textarea>
                <textarea name="input" rows="2" placeholder="Program stdin input (optional)">%s</textarea>
                <button>Compile and run</button>
            </form>
        </section>""" % (selected("c"), selected("cpp"), html.escape(code), html.escape(stdin)))
if report:
    print("        <section><h2>Result</h2><pre>%s</pre></section>" % html.escape(clip(report)))
print("    </main>\n</body>\n</html>")
