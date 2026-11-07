#!/usr/bin/env python3
"""EVAL_RULES.md statik kontrolleri.

Repo kokunden calistir:
    python3 tests/eval/static_checks.py            # G, F, IO (her push)
    python3 tests/eval/static_checks.py --teslim   # D, T, K-03 (teslim oncesi, elle)

Her satir bir kural ID'si tasir; ayrintisi EVAL_RULES.md'de.
"""
import glob
import os
import re
import subprocess
import sys
import time

SOURCES = sorted(glob.glob("src/**/*.cpp", recursive=True)
                 + glob.glob("includes/**/*.hpp", recursive=True) + ["main.cpp"])
results = []


def report(rid, ok, msg=""):
    ok = None if ok is None else bool(ok)
    tag = {True: "PASS", False: "FAIL", None: "WARN"}[ok]
    print(f"[{tag}] {rid}: {msg}")
    results.append((rid, ok))


def run(cmd):
    p = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def strip_comments(text):
    """Yorumlari ve string/char icini bosaltir; satir numaralari korunur."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j))
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            # string icerigi kalsin (method adlari gibi kontroller icin), sadece yorum degil
            out.append(text[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


CODE = {}
for path in SOURCES:
    with open(path, encoding="utf-8", errors="replace") as f:
        CODE[path] = strip_comments(f.read())


def find(pattern):
    """(dosya, satir, satir_metni) listesi; yorumlar haric."""
    rx = re.compile(pattern)
    hits = []
    for path, text in CODE.items():
        for no, line in enumerate(text.split("\n"), 1):
            if rx.search(line):
                hits.append((path, no, line.strip()))
    return hits


def fmt(hits, limit=10):
    return "; ".join(f"{p}:{n} `{t[:90]}`" for p, n, t in hits[:limit])


# ---------------------------------------------------------------- G: derleme

def check_build():
    mk = open("Makefile").read()
    flags_ok = (re.search(r"^CXX\s*=\s*c\+\+\s*$", mk, re.M)
                and all(f in mk for f in ("-Wall", "-Wextra", "-Werror")))
    report("G-01", bool(flags_ok), "Makefile: CXX = c++ ve -Wall -Wextra -Werror")

    run("make fclean")
    rc, out = run("make")
    report("G-01", rc == 0 and os.path.exists("webserv") and "warning:" not in out,
           "make temiz derliyor" if rc == 0 else f"make basarisiz:\n{out[-2000:]}")
    rc, out = run("make fclean && make CXXFLAGS='-Wall -Wextra -Werror -std=c++98'")
    report("G-01", rc == 0, "-std=c++98 ile derleniyor" if rc == 0 else out[-2000:])

    rc, out = run("make")
    relinked = [l for l in out.splitlines() if re.match(r"\s*c\+\+\s", l)]
    report("G-02", rc == 0 and not relinked,
           "ikinci make hicbir sey derlemiyor" if not relinked else f"relink: {relinked[:3]}")

    rules = all(re.search(rf"^{r}\s*:", mk, re.M) for r in (r"\$\(NAME\)", "all", "clean", "fclean", "re"))
    run("make clean")
    objs = [o for o in glob.glob("*.o") + glob.glob("src/**/*.o", recursive=True) + glob.glob("tests/*.o")]
    clean_ok = not objs and os.path.exists("webserv")
    run("make fclean")
    fclean_ok = not os.path.exists("webserv")
    run("make")
    only_main = os.path.exists("webserv") and not glob.glob("tests/*.o")
    run("make re")
    re_ok = os.path.exists("webserv")
    report("G-03", rules and clean_ok and fclean_ok and only_main and re_ok,
           f"kurallar={rules} clean={clean_ok} fclean={fclean_ok} make_sadece_webserv={only_main} re={re_ok}")


def check_libs():
    rc, out = run("ldd webserv")
    allowed = ("linux-vdso", "libstdc++", "libm.", "libgcc_s", "libc.", "ld-linux")
    bad = [l.strip() for l in out.splitlines() if l.strip() and not any(a in l for a in allowed)]
    report("G-04", rc == 0 and not bad, "sadece libc/libstdc++/libm/libgcc_s" if not bad else f"harici: {bad}")


def check_c_headers():
    hits = find(r"#\s*include\s*<(assert|ctype|errno|float|limits|locale|math|setjmp|signal|"
                r"stdarg|stddef|stdio|stdlib|string|time)\.h>")
    report("G-05", not hits, "C++ basliklari kullaniliyor" if not hits else fmt(hits))


# ---------------------------------------------------------------- F: fonksiyonlar

SUBJECT_FUNCS = set("""execve pipe strerror gai_strerror errno dup dup2 fork socketpair htons htonl
ntohs ntohl select poll epoll_create epoll_ctl epoll_wait kqueue kevent socket accept listen send
recv chdir bind connect getaddrinfo freeaddrinfo setsockopt getsockname getprotobyname fcntl close
read write waitpid kill signal access stat open opendir readdir closedir""".split())
GLIBC_ALIASES = {"fcntl64": "fcntl", "open64": "open", "stat64": "stat", "__xstat": "stat",
                 "__xstat64": "stat", "readdir64": "readdir", "__errno_location": "errno"}
# C90 kutuphanesi = C++98'in <cxxx> basliklari. system() fork eder, F-03 geregi disarida.
C90_FUNCS = set("""isalnum isalpha iscntrl isdigit isgraph islower isprint ispunct isspace isupper
isxdigit tolower toupper setlocale localeconv acos asin atan atan2 ceil cos cosh exp fabs floor fmod
frexp ldexp log log10 modf pow sin sinh sqrt tan tanh setjmp longjmp _setjmp raise remove rename
tmpfile tmpnam fclose fflush fopen freopen setbuf setvbuf fprintf fscanf printf scanf sprintf sscanf
vfprintf vprintf vsprintf fgetc fgets fputc fputs getc getchar gets putc putchar puts ungetc fread
fwrite fgetpos fseek fsetpos ftell rewind clearerr feof ferror perror atof atoi atol strtod strtol
strtoul rand srand calloc free malloc realloc abort atexit exit getenv bsearch qsort abs div labs
ldiv mblen mbtowc wctomb mbstowcs wcstombs memcpy memmove strcpy strncpy strcat strncat memcmp
strcmp strcoll strncmp strxfrm memchr strchr strcspn strpbrk strrchr strspn strstr strtok memset
strlen clock difftime mktime time asctime ctime gmtime localtime strftime""".split())
RUNTIME_PREFIXES = ("__cxa_", "_Unwind_", "_ITM_", "__isoc99_", "__isoc23_", "__ctype_")
RUNTIME_NAMES = {"__gxx_personality_v0", "__libc_start_main", "__gmon_start__", "__stack_chk_fail",
                 "__dso_handle", "__assert_fail", "__sigsetjmp", "__longjmp_chk"}


def allowed_symbol(sym):
    base = GLIBC_ALIASES.get(sym, sym)
    m = re.match(r"^__(\w+)_chk$", base)  # _FORTIFY_SOURCE varyantlari
    if m:
        base = m.group(1)
    if base.startswith(("__isoc99_", "__isoc23_")):
        base = base.split("_", 4)[-1]
    return (base in SUBJECT_FUNCS or base in C90_FUNCS or sym in RUNTIME_NAMES
            or sym.startswith(RUNTIME_PREFIXES))


def check_functions():
    rc, out = run("nm -u webserv")
    syms = sorted({l.split()[-1].split("@")[0] for l in out.splitlines()
                   if l.strip() and not l.split()[-1].startswith("_Z")})
    bad = [s for s in syms if not allowed_symbol(s)]
    where = []
    for s in bad:
        hits = find(rf"\b{re.escape(s)}\s*\(")
        where.append(f"{s} ({fmt(hits, 3) or 'kaynakta dogrudan cagri yok'})")
    report("F-01", rc == 0 and not bad, "sadece izinli fonksiyonlar" if not bad
           else "yasak: " + " | ".join(where))

    bad_fcntl = []
    for p, n, t in find(r"\bfcntl\s*\("):
        consts = set(re.findall(r"\b(F_[A-Z]+|O_[A-Z]+|FD_[A-Z]+)\b", t))
        if "F_SETFL" not in consts or not consts <= {"F_SETFL", "O_NONBLOCK", "FD_CLOEXEC"}:
            bad_fcntl.append((p, n, t))
    report("F-02", not bad_fcntl, "fcntl sadece F_SETFL/O_NONBLOCK/FD_CLOEXEC" if not bad_fcntl
           else fmt(bad_fcntl))

    forks = find(r"\bfork\s*\(")
    report("F-03", len(forks) == 1, f"{len(forks)} fork cagrisi (CGI yolunda olmali): {fmt(forks)}")
    execs = find(r"\bexecve\s*\(")
    report("F-04", len(execs) == 1, f"{len(execs)} execve cagrisi (sadece CGI): {fmt(execs)}")


# ---------------------------------------------------------------- IO: statik

def check_io():
    waits = find(r"\b(poll|select|epoll_wait|kevent)\s*\(")
    report("IO-01", len(waits) == 1, f"{len(waits)} bekleme cagrisi: {fmt(waits)}")

    errno_hits = [h for h in find(r"\berrno\b")
                  if re.sub(r"strerror\s*\(\s*errno\s*\)", "", h[2]).find("errno") >= 0]
    io_consts = find(r"\b(EAGAIN|EWOULDBLOCK|EINTR)\b")
    report("IO-05", not errno_hits and not io_consts,
           "errno sadece strerror() icinde" if not (errno_hits or io_consts)
           else fmt(errno_hits + io_consts))
    other = find(r"\bE(INPROGRESS|CONNRESET|PIPE|NOENT|ACCES|EXIST|NOTDIR|ISDIR|BADF|MFILE|NFILE|NOMEM)\b")
    if other:
        report("IO-05", None, "I/O disi errno sabiti, elle kontrol et (EVAL_RULES IO-05 risk): " + fmt(other))

    nonblock = find(r"\bO_NONBLOCK\b")
    report("IO-08", len(nonblock) > 0, f"O_NONBLOCK ayarlanan yerler: {fmt(nonblock)} "
           "(calisma zamani kontrolu e2e.py IO-08'de)")
    report("IO-03", None, "poll -> I/O yolu otomatik dogrulanamaz; read/recv/write/send cagri yerleri: "
           + fmt(find(r"\b(read|recv|write|send)\s*\("), 40))


# ---------------------------------------------------------------- teslim modu

def check_readme():
    if not os.path.exists("README.md"):
        report("D-01", False, "README.md yok (EvalHub: eksikse not 0)")
        return
    report("D-01", True, "README.md var")
    text = open("README.md", encoding="utf-8", errors="replace").read()
    first = text.split("\n", 1)[0].rstrip()
    rx = (r"^([*_])This project has been created as part of the 42 curriculum by "
          r"[^\s,]+(, [^\s,]+)*\.\1$")
    report("D-02", bool(re.match(rx, first)), f"ilk satir: {first!r}")
    for rid, word in (("D-03", "Description"), ("D-04", "Instructions"), ("D-05", "Resources")):
        report(rid, bool(re.search(rf"^#+\s*{word}\b", text, re.M)), f"'{word}' basligi")
    m = re.search(r"^#+\s*Resources\b(.*?)(?=^#\s|\Z)", text, re.M | re.S)
    ai = bool(m and re.search(r"\bAI\b|(?i:artificial intelligence|chatgpt|claude|copilot|\bllm\b)", m.group(1)))
    report("D-05", ai, "Resources icinde AI kullanimi anlatiliyor")
    report("D-06", None, "README Ingilizce mi: elle kontrol")


def check_files():
    files = subprocess.run(["git", "ls-files"], capture_output=True, text=True).stdout.split()
    remove_before, bad = [], []
    for f in files:
        if not os.path.exists(f):
            continue
        with open(f, "rb") as fh:
            magic = fh.read(4)
        ext = os.path.splitext(f)[1]
        if f.startswith(".github/") or f == "EVAL_RULES.md":
            remove_before.append(f)
        elif magic == b"\x7fELF" or ext in (".o", ".pdf", ".d"):
            bad.append(f)
        elif (f in ("Makefile", "README.md", ".gitignore") or ext in (".h", ".hpp", ".cpp", ".tpp", ".ipp", ".conf")
              or f.startswith(("html/", "tests/"))):
            continue
        else:
            bad.append(f)
    report("T-01", not bad, "takipli dosyalar teslim listesinde" if not bad
           else f"teslim listesinde olmayan {len(bad)} dosya: {bad}")
    if remove_before:
        report("T-01", None, f"teslimden once silinecek gelistirme dosyalari: {remove_before}")


def check_clean_configs():
    tracked = set(subprocess.run(["git", "ls-files"], capture_output=True, text=True).stdout.split())
    tracked_dirs = {os.path.dirname(f) for f in tracked}
    for conf in sorted(glob.glob("conf/*.conf")):
        text = re.sub(r"#[^\n]*", "", open(conf).read())
        missing = []
        for key, val in re.findall(r"\b(root|alias|upload_store|cgi\s+\S+)\s+([^;\s]+)\s*;", text):
            path = val.rstrip("/")
            if os.path.isabs(path):
                ok = os.path.exists(path)
            else:
                path = os.path.normpath(path)
                ok = path in tracked or any(d == path or d.startswith(path + "/") for d in tracked_dirs)
            if not ok:
                missing.append(f"{key} {val}")
        proc = subprocess.Popen(["./webserv", conf], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        time.sleep(1.5)
        alive = proc.poll() is None
        if alive:
            proc.terminate()
            proc.wait()
        err = proc.stderr.read().decode(errors="replace")[-300:] if not alive else ""
        report("T-02", alive and not missing,
               f"{conf}: " + ("aciliyor, yollar takipli" if alive and not missing
                              else f"eksik yol: {missing} " + (f"acilmadi: {err}" if not alive else "")))


def check_scope():
    methods = find(r'"(HEAD|PUT|OPTIONS|PATCH|TRACE|CONNECT)"')
    report("K-03", not methods, "sadece GET/POST/DELETE isleniyor" if not methods
           else "kapsam disi metot: " + fmt(methods))
    both = find(r'"root"') and find(r'"alias"')
    report("K-03", not both, "tek yol esleme mekanizmasi" if not both
           else "hem root hem alias var (ayni ozellik icin iki mekanizma); C-08'i saglayan kalsin")


def main():
    if "--teslim" in sys.argv:
        run("make")
        check_readme()
        check_files()
        check_clean_configs()
        check_scope()
    else:
        check_build()
        check_libs()
        check_c_headers()
        check_functions()
        check_io()
    fails = [r for r, ok in results if ok is False]
    print(f"\n{len(results)} kontrol, {len(fails)} FAIL: {sorted(set(fails))}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
