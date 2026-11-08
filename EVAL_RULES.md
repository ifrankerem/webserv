# Webserv Uyumluluk Denetimi

Bu dosya bir denetim şartnamesidir. Projedeki tüm kod ve dosyalar aşağıdaki kurallara göre test edilir.
Kurallar yalnızca Bölüm 1'deki üç kaynaktan türetilmiştir. Kaynaklarda olmayan hiçbir şey kural yapılmamıştır
ve kaynaklarda karşılığı olmayan hiçbir özellik kodda bulunmamalıdır (Bölüm 2).

---

## 0. Denetçi ajan için talimatlar

1. **Salt okunur çalış.** Kodu, config'leri, Makefile'ı, test dosyalarını değiştirme. Geçici config, site dosyası
   ve script'leri repo dışında bir dizine yaz (`D=$(mktemp -d)`). Upload testlerinin sunucuya yazdırdığı dosyaları
   test sonunda sil. Denetim bitince `git status` denetim öncesiyle aynı olmalı.
2. **Kaynak önceliği.** Bu dosya kaynakların özetidir. Bir kuralın anlamı şüpheliyse ilgili PDF'yi aç
   (`pdftotext -layout <pdf> -`). Bu dosya ile PDF çelişirse PDF kazanır; çelişkiyi raporda yaz.
3. **Kanıtsız GEÇTİ yok.** Her kural için kanıt ver: `dosya:satır`, çalıştırılan komut ve çıktısının kısa kesiti.
   Kanıt toplanamıyorsa (araç yok, test kurulamadı) sonuç `BELİRSİZ`'dir. BELİRSİZ, nihai kararda KALDI sayılır.
4. Bölüm 3'teki her kuralı sırayla değerlendir, hiçbirini atlama. Uygulanamıyorsa `UYGULANAMAZ` + gerekçe yaz.
5. Bölüm 2'deki kapsam envanterini çıkar.
6. Raporu Bölüm 5'teki şablonla ver.
7. Sunucuyu arka planda başlat, çıktısını geçici bir log dosyasına yönlendir, PID'yi kaydet. Her test grubundan sonra
   sürecin hâlâ yaşadığını (`kill -0 $PID`) ve normal bir `GET /` isteğinin 200 döndüğünü doğrula. Süreç ölmüşse
   o an çalışan test **R-01 KALDI (SIFIR)** olur.

### Sonuç değerleri

`GEÇTİ` · `KALDI` · `BELİRSİZ` · `UYGULANAMAZ`

### Ağırlıklar

| Ağırlık | Anlamı |
|---|---|
| **SIFIR** | Kaynak açıkça "grade 0", "evaluation stops" diyor ya da bir flag'e karşılık geliyor (Crash, Leaks, Forbidden function, Invalid compilation). Tek bir SIFIR KALDI = proje 0. E2'de "Check the code" bölümündeki her madde de SIFIR'dır ("If any point is unclear or is not correct, the evaluation stops"). |
| **BÖLÜM** | Eval sheet'teki ilgili Yes/No bölümünü "No" yaptırır. |
| **ZORUNLU** | Subject'te "must / must not / cannot / need / should" ile geçiyor, eval'de ayrı satırı yok. Yine de KALDI. |
| **KAPSAM** | Proje politikası: kaynaklarda karşılığı olmayan kod veya dosya. |

**Nihai karar:** herhangi bir KALDI veya BELİRSİZ varsa `UYUMSUZ`, yoksa `UYUMLU`. Bonus kuralları (B-xx) nihai
kararı etkilemez; sadece raporlanır.

### Kaynaklar çelişirse

Hepsini aynı anda sağlayan en katı davranış zorunludur. Aynı anda sağlanamıyorsa en yeni kaynak (S v24.0 ve
E2 09.2026) kazanır. Bilinen çelişkiler ve bağlayıcı yorum:

| Konu | Kaynaklar | Bağlayıcı yorum |
|---|---|---|
| İstemci başına I/O sayısı | E1: "only one read or one write per client per select". E2: "acceptable to read/write until EAGAIN/EWOULDBLOCK". | EAGAIN'i görmek errno okumayı gerektirir ve bu SIFIR'dır. Bu yüzden her readiness olayında fd başına **tek** read/recv veya **tek** write/send. Döngüyle tekrar okuma/yazma yok. |
| Virtual host | E1: farklı hostname'lerle çoklu sunucu test eder. S: "considered out of scope. But you are allowed to implement it". E2: "unless the team has chosen to implement the virtual host feature". | Opsiyonel. Uygulanmışsa çalışmak zorunda; uygulanmamışsa aynı interface:port tekrarı hata vermeli (C-04). |
| Siege kurulumu | E1: homebrew. E2: apt. | Fark yok; hangi paket yöneticisi varsa. |
| Index'siz dizin, listeleme kapalı | E1/E2 eki 42 tester: `GET /directory/Yeah` için "Expected 404". S: "NGINX may be used to compare" (NGINX 403 döner). | 404. Tester eval'in resmi eki; NGINX sadece karşılaştırma önerisi. |

---

## 1. Kaynaklar

| Kod | Dosya | Sürüm |
|---|---|---|
| **S** | `docs/PDF/en.subject.pdf` | Version 24.0 |
| **E1** | `docs/PDF/ng_5_webserv.pdf` (intra "Scale for project webserv") | 26.11.2021 |
| **E2** | `docs/PDF/42 EvalHub.pdf` | Last update 09.2026 |

- `docs/subject.txt` ve `docs/PDF/tr.subject.pdf` kaynak değildir. Çelişkide S kazanır.
- S'nin "AI Instructions" bölümü kod kuralı içermez; Q-02'de insan kontrolü olarak geçer.

---

## 2. Kapsam politikası

### K-01 — Her özellik bir kurala izlenebilir olmalı — KAPSAM

Koddaki her özellik (config direktifi, HTTP metodu, işlenen header, CGI davranışı, Makefile hedefi, repodaki dosya)
Bölüm 3'teki en az bir kural ID'sine bağlanabilmeli. Bağlanamayan her özellik ayrı bir KAPSAM bulgusudur.

### K-02 — Kaynakların açıkça izin verdiği ekler (bulgu değil)

| Ek | Dayanak |
|---|---|
| `poll()` yerine `select`, `epoll`, `kqueue` | S: "you can use any equivalent function such as select(), kqueue(), or epoll()" |
| Virtual host ve `server_name` | S: "you are allowed to implement it", "a server name for a website if you plan to implement virtual hosts". **Şart:** virtual host gerçekten çalışmalı (C-04). Parse edilip etkisi olmayan `server_name` = KAPSAM bulgusu. |
| Bir subject özelliğine hizmet eden ek config bilgisi | S: "You can have other rules or configuration information in your file". **Şart:** direktifin gözle görülür bir etkisi olmalı ve aynı özellik için ikinci bir mekanizma olmamalı (K-03). |
| Test kaynakları ve test script'leri | S: "Write your tests in a more suitable language, such as Python or Golang, among others, even in C or C++". **Şart:** `make` / `make all` bunları derlemez. |
| Makefile'da zorunlulara ek hedefler | S: "Your Makefile must at least contain the rules". **Şart:** `make` / `make all` yalnızca `webserv` üretir. |
| Bonus: cookie/session, çoklu CGI | S Bonus part |
| stdout/stderr'e log, errno'yu loglamak | E2: "errno may be logged, not used to drive control flow"; "an error should be displayed if an issue occurs" |
| Tarayıcı uyumu için gereken HTTP davranışı: status line, `Content-Length`, `Content-Type` (MIME), `Connection`, `Location`, `Host` okuma, chunked çözme, ham body upload | S: "compatible with standard web browsers", "Clients must be able to upload files", "for chunked requests, your server needs to un-chunk them", RFC okuma tavsiyesi |

### K-03 — Bulgu sayılan ekler — KAPSAM

Aşağıdakiler kaynaklarda yoktur, kodda bulunursa bulgudur (liste örnektir, tam değildir):

- GET, POST, DELETE dışında bir metodu işleyen kod (HEAD, PUT, OPTIONS, PATCH, TRACE, CONNECT...). S "at least" der;
  proje politikası gereği bu üçü dışındaki metotlar işlenmez, H-03 tablosundaki kodla reddedilir.
- Aynı özellik için iki mekanizma. Örnek: yol eşleme için hem `root` hem `alias`, farklı semantikle. Subject
  örneğini (C-08) sağlayan mekanizma kalır, diğeri bulgudur.
- Parse edilen ama hiçbir davranışı değiştirmeyen config direktifi.
- Range, conditional GET (`If-Modified-Since`, `ETag`), sıkıştırma, cache header'ları, auth, HTTPS, proxy,
  rewrite/regex location, CGI dışı dinamik içerik.
- Not: zaman aşımları ve boyut sınırları R-02 / H-03'e hizmet ettiği sürece bulgu değildir.
- Teslim listesinde olmayan takipli dosyalar (T-01).

### K-04 — Kapsam envanteri prosedürü

Ajan aşağıdaki listeleri çıkarır ve her satırı bir kurala eşler:

1. **Config direktifleri:** `grep -ohE '"[a-z_]+"' parser/*.cpp | sort -u`. Her direktif için: hangi kurala hizmet
   ettiği ve etkisinin kodda nerede uygulandığı (`dosya:satır`).
2. **HTTP metotları:** `grep -rnE '"(GET|POST|DELETE|HEAD|PUT|OPTIONS|PATCH|TRACE|CONNECT)"' src parser includes main.cpp`.
3. **İşlenen request header'ları:** kodda adı geçen her header (`Host`, `Content-Length`, `Transfer-Encoding`,
   `Content-Type`, `Connection`, `Cookie`...) ve hizmet ettiği kural.
4. **Üretilen response header'ları:** her biri ve hizmet ettiği kural.
5. **Makefile hedefleri:** her hedef ve dayanağı.
6. **Kaynak dosyalar:** her `.cpp/.hpp` dosyası ve hizmet ettiği kurallar.
7. **Takipli dosyalar:** `git ls-files` → T-01.

---

## 3. Kurallar

Her kural: **Kaynak** (alıntı), **Kontrol** (komut veya inceleme), **Geçme kriteri**.

### 3.1 Derleme ve Makefile (G)

#### G-01 — Derleyici, bayraklar, C++98 — SIFIR (Invalid compilation)
- **Kaynak:** S Ch.II: "Compile your code with c++ and the flags -Wall -Wextra -Werror", "comply with the C++ 98
  standard and should still compile when adding the flag -std=c++98".
- **Kontrol:** Makefile'da `CXX = c++` ve `-Wall -Wextra -Werror` var mı. `make fclean && make` hatasız ve uyarısız.
  `make fclean && make CXXFLAGS="-Wall -Wextra -Werror -std=c++98"` hatasız.
- **Geçme:** iki derleme de başarılı, `./webserv` oluşuyor.

#### G-02 — Relink yok — SIFIR (Invalid compilation)
- **Kaynak:** S: "It must not perform unnecessary relinking." E2: "The project must compile without any relinking issues."
- **Kontrol:** `make && make` ikinci çağrının çıktısı; `make -n` (build sonrası).
- **Geçme:** ikinci `make` hiçbir derleme veya link komutu çalıştırmıyor.

#### G-03 — Zorunlu Makefile kuralları — ZORUNLU
- **Kaynak:** S: "Your Makefile must at least contain the rules: $(NAME), all, clean, fclean and re."
- **Kontrol:** her kuralı çalıştır. `clean` → `.o` dosyaları silinir, binary kalır. `fclean` → binary de silinir.
  `re` → sıfırdan derler. `make` (hedefsiz) → yalnızca `webserv`.
- **Geçme:** beş kural var ve tarif edildiği gibi çalışıyor.

#### G-04 — Harici kütüphane yok — ZORUNLU
- **Kaynak:** S: "Any external library and Boost libraries are forbidden."
- **Kontrol:** link satırında `-l...` yok; `#include`'larda standart C++/POSIX dışı başlık yok; `ldd webserv` yalnızca
  libc, libstdc++, libm, libgcc_s, ld-linux, linux-vdso gösteriyor.
- **Geçme:** üçü de temiz.

#### G-05 — C başlıkları yerine C++ başlıkları — ZORUNLU
- **Kaynak:** S: "choose <cstring> over <string.h>", "always prefer their C++ versions if possible".
- **Kontrol:**
  `grep -rnE '#include *<(assert|ctype|errno|float|limits|locale|math|setjmp|signal|stdarg|stddef|stdio|stdlib|string|time)\.h>' src parser includes main.cpp`
- **Geçme:** sonuç boş. C++ karşılığı olmayan POSIX başlıkları (`unistd.h`, `sys/*.h`, `netinet/in.h`,
  `arpa/inet.h`, `fcntl.h`, `dirent.h`, `netdb.h`, `poll.h`) serbesttir.

### 3.2 İzinli fonksiyonlar (F)

#### F-01 — Yalnızca izinli dış fonksiyonlar — SIFIR (Forbidden function)
- **Kaynak:** S Ch.IV "External Function":
  `execve, pipe, strerror, gai_strerror, errno, dup, dup2, fork, socketpair, htons, htonl, ntohs, ntohl, select,
  poll, epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), socket, accept, listen, send, recv,
  chdir, bind, connect, getaddrinfo, freeaddrinfo, setsockopt, getsockname, getprotobyname, fcntl, close, read,
  write, waitpid, kill, signal, access, stat, open, opendir, readdir, closedir`.
  Ayrıca "All functionality must be implemented in C++ 98" → C++98 standart kütüphanesi (C90 kütüphanesi `<cxxx>`
  başlıkları dahil) serbesttir.
- **Kontrol:** `nm -u webserv | grep -v ' _Z' | awk '{print $2}' | sed 's/@.*//' | sort -u`
  Her sembol şu üç gruptan birine girmeli:
  1. Yukarıdaki izin listesi. glibc takma adları aynı fonksiyon sayılır: `fcntl64`, `open64`, `stat64`, `__xstat`,
     `readdir64`, `__errno_location` (= errno).
  2. C90 standart kütüphanesi (`<cassert> <cctype> <cerrno> <cfloat> <climits> <clocale> <cmath> <csetjmp>
     <csignal> <cstdarg> <cstddef> <cstdio> <cstdlib> <cstring> <ctime>`) ve glibc iç adları (`__isoc99_*`,
     `__isoc23_*`, `__ctype_*_loc`, `__assert_fail`).
  3. Derleyici/çalışma zamanı: `__cxa_*`, `_Unwind_*`, `__gxx_personality_v0`, `__libc_start_main`,
     `__gmon_start__`, `__stack_chk_fail`, `__dso_handle`, `_ITM_*`.
- **Sık yakalanan yasaklar** (C90'da yok, listede yok): `strdup`, `inet_addr`, `inet_ntoa`, `inet_pton`, `inet_ntop`,
  `_exit`, `unlink`, `rmdir`, `mkdir`, `getcwd`, `realpath`, `fstat`, `lstat`, `gettimeofday`, `clock_gettime`,
  `sleep`, `usleep`, `nanosleep`, `sigaction`, `wait`, `execvp`, `execlp`, `pipe2`, `accept4`, `epoll_create1`,
  `recvfrom`, `sendto`, `setenv`, `putenv`, `getpid`, `strerror_r`, `pthread_*`.
- **Geçme:** listede olmayan tek sembol yok.

#### F-02 — fcntl yalnızca izinli bayraklarla — ZORUNLU
- **Kaynak:** S IV.2: "you are allowed to use fcntl() only with the following flags: F_SETFL, O_NONBLOCK and,
  FD_CLOEXEC. Any other flag is forbidden."
- **Kontrol:** `grep -rn 'fcntl' src parser includes main.cpp`
- **Geçme:** her çağrı `fcntl(fd, F_SETFL, O_NONBLOCK)` biçiminde (isteğe bağlı `| FD_CLOEXEC`). `F_GETFL`,
  `F_SETFD`, `F_GETFD` veya başka bayrak yok.

#### F-03 — fork yalnızca CGI için — ZORUNLU
- **Kaynak:** S: "You can't use fork for anything other than CGI".
- **Kontrol:** `grep -rn 'fork' src parser includes main.cpp`
- **Geçme:** tek kullanım CGI başlatma yolunda.

#### F-04 — Başka web sunucusu çalıştırılmaz — ZORUNLU
- **Kaynak:** S: "You cannot execve another web server."
- **Kontrol:** `grep -rn 'execve' src parser includes main.cpp`
- **Geçme:** `execve` yalnızca CGI'da, config'teki yorumlayıcı/script ile çağrılıyor.

### 3.3 I/O ve event loop (IO) — hepsi SIFIR

#### IO-01 — Tek poll (veya eşdeğeri)
- **Kaynak:** S: "use only 1 poll() (or equivalent) for all the I/O operations between the clients and the server
  (listen included)". E2: "The poll() (or equivalent) should be in the main loop".
- **Kontrol:** `grep -rnE '\b(poll|select|epoll_wait|kevent)\s*\(' src parser includes main.cpp`
- **Geçme:** tek çağrı noktası, ana döngüde; listen soketleri, istemci soketleri ve CGI pipe'ları aynı kümede.

#### IO-02 — Okuma ve yazma aynı anda izlenir
- **Kaynak:** S: "poll() (or equivalent) must monitor both reading and writing simultaneously." E2: "must check file
  descriptors for both reading and writing simultaneously. Otherwise, the grade is 0".
- **Kontrol:** tek bekleme çağrısının hem okuma hem yazma hazırlığını aynı turda döndürebildiğini kodda göster.
- **Geçme:** aynı çağrı ikisini de raporluyor. Yazacak veri varken yazma ilgisinin (EPOLLOUT/POLLOUT) açılıp iş bitince
  kapatılması kabul. Okuma ve yazma için ayrı bekleme çağrıları = KALDI.

#### IO-03 — Hazırlık olmadan I/O yok
- **Kaynak:** S: "You must never do a read or a write operation without going through poll()", "Calling read/recv
  or write/send on these descriptors without prior readiness will result in a grade of 0. Regular disk files are
  exempt." E2: "Writing or reading event-driven descriptors (sockets, pipes/FIFOs, TTY, etc.) without going through
  poll() (or equivalent) is strictly FORBIDDEN".
- **Kontrol:** soket veya pipe üzerindeki her `accept/read/recv/write/send` çağrısını listele. Her biri için:
  fd türü, tetikleyen olay (okuma için IN, yazma için OUT, HUP/ERR) ve poll sonucundan o çağrıya giden yol.
  Bu tablo raporda zorunludur ("I/O yol haritası"). Özellikle bak:
  - istek okunduktan hemen sonra aynı turda yanıt gönderme,
  - fork'tan hemen sonra CGI stdin'ine body yazma,
  - `waitpid` sonrası CGI çıktısını bloklayarak okuma,
  - hata yanıtını (413, 400...) okuma anında doğrudan `send` etme,
  - yıkıcıda (destructor) veya kapatma yolunda `send`.
- **Geçme:** her çağrı, o fd için o olay türünün hazır olduğu bir poll sonucundan geliyor. Normal disk dosyaları
  (statik dosya okuma, upload yazma) muaf.

#### IO-04 — Olay başına fd başına tek I/O
- **Kaynak:** E1: "There should be only one read or one write per client per select". Çelişki yorumu için Bölüm 0.
- **Kontrol:** soket/pipe `recv/read/send/write` çağrılarının bir döngü içinde tekrarlanıp tekrarlanmadığını incele.
- **Geçme:** her readiness olayında fd başına en fazla bir okuma veya bir yazma.

#### IO-05 — I/O sonrası errno kontrolü yok
- **Kaynak:** S: "Checking the value of errno to adjust the server behaviour is strictly forbidden after performing
  a read or write operation." E2: "If errno is checked after read/recv/write/send, the grade is 0 ... (errno may be
  logged, not used to drive control flow)".
- **Kontrol:** `grep -rnE 'errno|EAGAIN|EWOULDBLOCK|EINTR|EINPROGRESS' src parser includes main.cpp`
- **Geçme:** errno yalnızca loglamada (`strerror(errno)` çıktıya yazılıyor). Hiçbir `if/switch/while` errno değerine
  bakmıyor. I/O dışındaki bir çağrıdan sonra errno'ya bakan dal varsa raporda "risk" olarak yaz.

#### IO-06 — Dönüş değerleri tam kontrol
- **Kaynak:** E1/E2: "Search for all read/recv/write/send and check if the returned value is correctly handled
  (checking only -1 or only 0 is not enough; both must be handled)."
- **Kontrol:** her soket/pipe `read/recv/write/send` çağrısı için `< 0` ve `== 0` dallarını göster.
- **Geçme:** okumada `0` (karşı taraf kapattı / pipe EOF) ve `< 0` ayrı ayrı ele alınıyor. Yazmada `< 0` ve `0` ele
  alınıyor; kısmi yazmada kalan veri bir sonraki OUT olayında gönderiliyor.

#### IO-07 — Hata olunca istemci silinir
- **Kaynak:** E1/E2: "Search for all read/recv/write/send on a socket and check that, if an error is returned, the
  client is removed." S: "properly handle client disconnections when necessary".
- **Kontrol:** hata dallarında fd kapatılıyor mu, poll kümesinden çıkarılıyor mu, istemci nesnesi siliniyor mu,
  bağlı CGI süreci öldürülüp `waitpid` ile toplanıyor mu.
  Çalışma zamanı: `ls /proc/$PID/fd | wc -l` taban değerini al; 50 kez büyük dosya indirmesini başlatıp yarıda kes
  (`timeout 0.3 curl -s -o /dev/null URL`); sayı taban değerine dönmeli.
- **Geçme:** dördü de var ve fd sayısı tabana dönüyor.

#### IO-08 — Her şey non-blocking
- **Kaynak:** S: "Your server must remain non-blocking at all times", "I/O that can wait for data (sockets,
  pipes/FIFOs, etc.) must be non-blocking".
- **Kontrol:** listen soketi, her `accept` edilen soket ve CGI pipe'larının sunucu tarafı uçları için `O_NONBLOCK`
  ayarlandığı satırı göster.
- **Geçme:** üç fd türünün hepsi non-blocking.

#### IO-09 — Disk I/O event loop'u kilitlemez
- **Kaynak:** E2: "Confirm that synchronous I/O on regular disk files (e.g., reading the configuration or small static
  files) does not stall the event loop."
- **Kontrol:** 200 MB'lık bir dosya oluştur. Arka planda `curl --limit-rate 1M -o /dev/null URL/buyuk` başlat; aynı anda
  küçük bir dosya iste: `curl -s -o /dev/null -w '%{time_total}\n' URL/kucuk.html`. Aynısını 200 MB'lık upload
  sürerken tekrarla.
- **Geçme:** küçük istek her iki durumda da 1 saniyenin altında döner.

### 3.4 Dayanıklılık (R)

#### R-01 — Asla çökmez — SIFIR (Crash)
- **Kaynak:** S: "Your program must not crash under any circumstances (even if it runs out of memory) or terminate
  unexpectedly." E1/E2: "no segfault or any other unexpected, premature, or uncontrolled termination".
- **Kontrol:** her denemeden sonra `kill -0 $PID` ve normal `GET /` → 200. Denemeler:
  - rastgele bayt: `head -c 10000 /dev/urandom | nc 127.0.0.1 PORT`
  - 100 KB'lık URI, 100 KB'lık tek header, 10 000 header satırı
  - versiyonsuz istek satırı, boş istek satırı, sadece `\r\n\r\n`
  - `Content-Length: -1`, `Content-Length: 99999999999999999999`, `Content-Length: abc`, iki farklı `Content-Length`
  - geçersiz chunk boyutu (`zz`, `-5`, `FFFFFFFFFFFFFFFFFFFF`), bitmeyen chunked body
  - `GET /../../../../etc/passwd`, `GET /%2e%2e/%2e%2e/etc/passwd`, `GET /a%00b`
  - istek yarıdayken bağlantıyı kes; yanıt yarıdayken bağlantıyı kes (SIGPIPE): 50 kez
    `timeout 0.2 curl -s -o /dev/null URL/buyuk`
  - fd limitini aşacak kadar eşzamanlı bağlantı (ör. `ulimit -n` değerinin iki katı kadar `nc` açık bırak)
  - var olmayan CGI script'i, çalıştırılamayan script, var olmayan yorumlayıcı, stdout'u erken kapatan script,
    100 MB çıktı üreten script
  - bellek sınırı: sunucuyu `prlimit --as=300000000 ./webserv <conf>` ile başlat, 50 MB'lık POST'lar gönder
  - DELETE ile dizin silmeye çalış
- **Geçme:** sunucu her denemeden sonra yaşıyor ve sonraki normal istek 200 dönüyor.

#### R-02 — İstek asla sonsuza kadar asılı kalmaz — ZORUNLU
- **Kaynak:** S: "A request to your server should never hang indefinitely." E2: "Check that there are no hanging
  connections."
- **Kontrol:** koddaki zaman aşımı değerini (T) bul ve raporla. Sonra:
  - boş bağlantı: `time (sleep 300 | nc 127.0.0.1 PORT)` (hiçbir şey gönderme)
  - yarım header: `time ( (printf 'GET / HTTP/1.1\r\nHost: x\r\n'; sleep 300) | nc 127.0.0.1 PORT )`
  - yarım body: `Content-Length: 100` gönder, 10 bayt yolla, bekle
  - sonsuz döngülü CGI (H-03 / CGI-07)
  - yanıtı hiç okumayan istemci: büyük dosya iste, soketi okuma
- **Geçme:** her durumda bağlantı en geç T + 5 sn içinde kapanıyor (uygunsa 408 / 504 ile). T sonlu bir değer.

### 3.5 Config (C)

#### C-01 — Config argümanla veya varsayılan yoldan — ZORUNLU
- **Kaynak:** S: "Your program must use a configuration file, provided as an argument on the command line, or
  available in a default path." Kullanım: `./webserv [configuration file]`.
- **Kontrol:** `./webserv conf/<dosya>.conf`; `./webserv` (argümansız, repo kökünden); `./webserv a b`.
- **Geçme:** ilk ikisi çalışıyor (varsayılan yol repoda mevcut), üçüncüsü kullanım hatası basıp çöküşsüz çıkıyor.

#### C-02 — Geçersiz config çöküş yaratmaz — SIFIR (Crash)
- **Kaynak:** S Ch.II (asla çökmez).
- **Kontrol:** `tests/test_configs/invalid_cases/` altındaki her dosya; var olmayan dosya; dizin; boş dosya;
  okunamayan dosya (`chmod 000`). Her biri için çıkış kodu ve stderr.
- **Geçme:** her birinde anlaşılır hata mesajı, sinyalsiz çıkış (çıkış kodu < 128), açık kalan dinleme soketi yok.

#### C-03 — interface:port çiftleri, farklı siteler — BÖLÜM (Configuration, Port issues)
- **Kaynak:** S: "Define all the interface:port pairs on which your server will listen to", "listen to multiple
  ports to deliver different content". E2: "Set up multiple websites on different interfaces and ports."
- **Kontrol:** üç server bloğu: `127.0.0.1:A`, `127.0.0.1:B`, `127.0.0.2:A`, her biri farklı kök dizin ve farklı
  `index.html`. Her birine `curl`.
- **Geçme:** her adres kendi içeriğini döndürüyor.

#### C-04 — Aynı interface:port tekrarı — BÖLÜM (Port issues)
- **Kaynak:** E2: "Configure multiple websites on the same interface:port. This should result in an error, unless the
  team has chosen to implement the virtual host feature." E1: "setup multiple servers with different hostname (use
  something like: curl --resolve example.com:80:127.0.0.1 http://example.com/)".
- **Kontrol:** aynı `interface:port` ile iki server bloğu, farklı `server_name` ve farklı içerik.
- **Geçme (virtual host yoksa):** sunucu başlangıçta config hatası verip çöküşsüz çıkıyor.
- **Geçme (virtual host varsa):** `curl --resolve ad1:PORT:127.0.0.1 http://ad1:PORT/` ve `ad2` için ayrı içerik;
  bilinmeyen Host → o port için ilk tanımlanan server. Virtual host yokken `server_name` parse ediliyorsa → K-03.

#### C-05 — Varsayılan hata sayfaları — ZORUNLU
- **Kaynak:** S: "Your server must have default error pages if none are provided."
- **Kontrol:** hiç `error_page` içermeyen bir config ile H-03 tablosundaki her hata durumunu üret.
- **Geçme:** her hata yanıtı doğru status, `Content-Type: text/html` ve boş olmayan bir HTML gövdesi içeriyor.

#### C-06 — Özel hata sayfası — BÖLÜM (Configuration)
- **Kaynak:** S: "Set up default error pages." E1/E2: "try modifying the 404 error page".
- **Kontrol:** `error_page 404 <yol>` ile config; olmayan bir URL iste. Sonra sayfanın dosyasını sil ve tekrar iste.
- **Geçme:** özel içerik **404 status ile** dönüyor (200 değil). Dosya yoksa varsayılan sayfa yine 404 ile dönüyor;
  döngü veya çöküş yok.

#### C-07 — client_max_body_size — BÖLÜM (Configuration)
- **Kaynak:** S: "Set the maximum allowed size for client request bodies." E2:
  `curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit"`.
- **Kontrol:** limit 20 bayt olan, POST'a izin veren bir route. Gönder: 10 bayt, tam 20 bayt, 21 bayt, 21 baytlık
  chunked body (`-H 'Transfer-Encoding: chunked'`).
- **Geçme:** 10 ve 20 bayt 413 almıyor; 21 bayt ve chunked 21 bayt 413 alıyor; sunucu bağlantıyı düzgün kapatıyor.

#### C-08 — Route → dizin eşlemesi — BÖLÜM (Configuration)
- **Kaynak:** S: "if URL /kapouet is rooted to /tmp/www, URL /kapouet/pouic/toto/pouet will search for
  /tmp/www/pouic/toto/pouet". E2: "Set up routes on a server to different directories."
- **Kontrol:** `$D/www/pouic/toto/pouet` dosyasını oluştur. `/kapouet` location'ını projenin yol eşleme direktifiyle
  `$D/www`'ya bağla. `GET /kapouet/pouic/toto/pouet` iste. İkinci bir yol eşleme direktifi varsa onu da dene.
  Ek olarak kökten kaçma denemeleri (R-01 listesindeki `..` ve `%2e%2e` istekleri).
- **Geçme:** dosya 200 ile, birebir içerikle dönüyor; kök dışına hiçbir istek çıkamıyor (403/404/400). Aynı iş için
  farklı semantikli ikinci bir direktif varsa K-03 bulgusu yaz.

#### C-09 — Dizin için varsayılan dosya — BÖLÜM (Configuration)
- **Kaynak:** S: "Default file to serve when the requested resource is a directory." E2: "Set up a default file to
  serve when requesting a directory."
- **Kontrol:** `index` tanımlı bir dizine `GET /dizin/`.
- **Geçme:** index dosyasının içeriği 200 ile dönüyor.

#### C-10 — Route başına izinli metotlar — BÖLÜM (Configuration)
- **Kaynak:** S: "List of accepted HTTP methods for the route." E2: "try DELETE something with and without permission".
- **Kontrol:** aynı dosyayı DELETE'e izin vermeyen ve izin veren iki route üzerinden sil.
- **Geçme:** izinsiz route 405 döner ve dosya yerinde kalır; izinli route dosyayı siler (H-06).

#### C-11 — HTTP yönlendirme — BÖLÜM (Configuration, Check with a browser)
- **Kaynak:** S: "HTTP redirection." E1/E2: "Try a redirected URL."
- **Kontrol:** `curl -si URL/eski`; `curl -siL URL/eski`.
- **Geçme:** config'teki 3xx kodu ve `Location` header'ı dönüyor; `-L` hedefe ulaşıyor; tarayıcı yönleniyor.

#### C-12 — Dizin listeleme açık/kapalı — BÖLÜM (Check with a browser)
- **Kaynak:** S: "Enabling or disabling directory listing." E1/E2: "Try to list a directory."
- **Kontrol:** index dosyası olmayan iki dizin: biri listeleme açık, biri kapalı.
- **Geçme:** açık → 200, dizindeki her girişi link olarak gösteren HTML. Kapalı → 404 (bkz. Bölüm 0 çelişki tablosu).

#### C-13 — Upload izni ve depolama yeri — ZORUNLU
- **Kaynak:** S: "Uploading files from the clients to the server is authorized, and storage location is provided."
- **Kontrol:** upload açık route'a dosya yükle; dosyanın config'teki depolama dizinine yazıldığını `ls` ile doğrula.
  Upload kapalı bir route'a aynı isteği gönder.
- **Geçme:** dosya tam olarak tanımlı dizine yazılıyor; kapalı route 4xx dönüyor ve diske hiçbir şey yazmıyor.

#### C-14 — Her özellik için config ve demo dosyası repoda — ZORUNLU
- **Kaynak:** S: "You must provide configuration files and default files to test and demonstrate that every feature
  works during the evaluation."
- **Kontrol:** C-03..C-13, H-04..H-07, CGI-01..CGI-07 için repodaki hangi config'in ve hangi demo dosyasının o
  özelliği gösterdiğini tablo olarak çıkar. Repoyu boş bir dizine `git clone` et, `make`, her config'i başlat.
- **Geçme:** her özelliğin repoda takipli bir config'i ve demo dosyası var; temiz klondan her config eksik yol
  hatası vermeden açılıyor.

### 3.6 HTTP davranışı (H)

#### H-01 — GET, POST, DELETE çalışır — BÖLÜM (Basic checks)
- **Kaynak:** S: "You need at least the GET, POST, and DELETE methods." E2: "GET, POST, and DELETE requests should work."
- **Kontrol:** `curl` ve `nc`/`telnet` ile ham istekler.
- **Geçme:** üçü de H-03'teki doğru kodla çalışıyor.

#### H-02 — Bilinmeyen metot çöküş yaratmaz — BÖLÜM (Basic checks)
- **Kaynak:** E1/E2: "UNKNOWN requests should not result in a crash."
- **Kontrol:** `printf 'FOO / HTTP/1.1\r\nHost: x\r\n\r\n' | nc 127.0.0.1 PORT`; aynısını `PUT`, `HEAD`, `PATCH` ile.
- **Geçme:** sunucu yaşıyor ve H-03'teki kod dönüyor.

#### H-03 — Status kodları doğru — BÖLÜM (her ilgili bölüm)
- **Kaynak:** S: "Your HTTP response status codes must be accurate." E1/E2: "if any status codes are incorrect, do
  not award any points related to them."
- **Kontrol ve beklenen değerler:**

| Durum | Beklenen |
|---|---|
| Var olan dosyaya GET | 200 |
| Olmayan yola GET | 404 |
| Dizin, index var | 200 (index içeriği) |
| Dizin, index yok, listeleme açık | 200 (liste) |
| Dizin, index yok, listeleme kapalı | 404 (42 tester; Bölüm 0) |
| Okuma izni olmayan dosya (`chmod 000`) | 403 |
| GET/POST/DELETE ama route izin vermiyor | 405 |
| Bu üçü dışında metot (`FOO`, `PUT`, `HEAD`...) | 501 (RFC 9110 §15.6.2). 405 da kabul (NGINX davranışı, S: "NGINX may be used to compare"). Başka kod KALDI. |
| Bozuk istek satırı veya header | 400 |
| HTTP/1.1 isteğinde `Host` yok | 400 |
| `Content-Length` ile `Transfer-Encoding` birlikte | 400 |
| Desteklenmeyen sürüm (`HTTP/2.0`, `HTTP/3.0`) | 505 |
| Body var, `Content-Length` yok, chunked değil | 411 |
| Body, `client_max_body_size`'dan büyük | 413 |
| URI sunucunun sınırından uzun (sınır varsa) | 414 |
| Header'lar sunucunun sınırından büyük (sınır varsa) | 431 veya 400 |
| Upload ile yeni dosya oluştu | 201 |
| DELETE başarılı | 204 (gövdesiz) veya 200 (gövdeli) |
| Olmayan dosyaya DELETE | 404 |
| Dosya sistemi izni olmayan dosyaya DELETE | 403 |
| Config'teki yönlendirme | config'teki 3xx + `Location` |
| İstemci zaman aşımı (yarım istek) | 408 veya yanıtsız kapatma |
| CGI zaman aşımı (sonsuz döngü) | 504 |
| CGI geçersiz çıktı / çöktü / çıktısız sonlandı | 502 veya 500 |
| CGI script'i yok | 404 |
| Sunucu iç hatası | 500 |

- **Geçme:** her satır beklenen kodu veriyor. Her hata yanıtında C-05/C-06 sayfası var.

#### H-04 — Statik site tarayıcıda — BÖLÜM (Check with a browser)
- **Kaynak:** S: "You must be able to serve a fully static website", "compatible with standard web browsers".
  E1/E2: "Open the network tab", "Look at the request header and response header", "Try an incorrect URL", "Try
  anything you like".
- **Kontrol:** HTML + CSS + JS + resim (png, jpg, gif, svg, ico) içeren bir sayfa. `curl -sI` ile her dosya türünün
  `Content-Type`'ı; `curl -s URL | wc -c` ile `Content-Length` eşitliği. Tarayıcı otomasyonu varsa sayfayı aç,
  network sekmesinde başarısız istek ve console'da MIME hatası olmadığını doğrula.
- **Geçme:** her tür doğru MIME ile, doğru uzunlukla geliyor; sayfa eksiksiz yükleniyor; yanlış URL hata sayfası
  gösteriyor.

#### H-05 — Upload ve geri alma — BÖLÜM (Basic checks)
- **Kaynak:** S: "Clients must be able to upload files." E2: "Upload some files to the server and retrieve them."
- **Kontrol:** `head -c 1048576 /dev/urandom > $D/r.bin`; bir PNG; bir metin dosyası. Her birini
  `curl --data-binary @...` (ham body) ile yükle, `GET` ile geri al, `cmp`.
  Tarayıcıdan `html/index.html` test sayfasıyla da yükle (dosyayı ham body olarak yollar).
- **Geçme:** her dosya bayt bayt aynı geri geliyor. Multipart desteklenmiyor: multipart gövde olduğu gibi kaydedilir.

#### H-06 — DELETE — BÖLÜM (Basic checks)
- **Kaynak:** S, E2 (H-01, C-10).
- **Kontrol:** yüklenen dosyayı izinli route'tan sil, sonra `GET`.
- **Geçme:** DELETE 204/200, sonraki GET 404. İzinsiz route ve izinsiz dosya durumları H-03'e göre.

#### H-07 — Chunked istek çözülür — ZORUNLU
- **Kaynak:** S: "for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the
  body."
- **Kontrol:** `curl -H 'Transfer-Encoding: chunked' --data-binary @$D/r.bin` ile upload ve CGI'a POST.
- **Geçme:** saklanan dosya ve CGI'ın stdin'den okuduğu body, chunk çerçevesi olmadan orijinalle aynı.

#### H-08 — Yanıt biçimi — ZORUNLU
- **Kaynak:** S: "NGINX may be used to compare headers and answer behaviours", "compatible with standard web browsers".
- **Kontrol:** `printf 'GET / HTTP/1.1\r\nHost: x\r\n\r\n' | nc 127.0.0.1 PORT | head -20`; NGINX kuruluysa aynı
  isteğin NGINX yanıtıyla karşılaştırması.
- **Geçme:** `HTTP/1.x <kod> <açıklama>\r\n` status satırı, CRLF ayraçlı header'lar, gövde uzunluğu ya
  `Content-Length` ile ya da bağlantı kapanışıyla belirli.

#### H-09 — Bağlantı yönetimi tutarlı — ZORUNLU
- **Kaynak:** S: "A request to your server should never hang indefinitely", tarayıcı uyumu.
- **Kontrol:** `curl -sv URL/a URL/b` (aynı bağlantıyı yeniden kullanmayı dener).
- **Geçme:** iki istek de doğru yanıt alıyor. Sunucu bağlantıyı tutuyorsa sonraki istek çalışıyor; kapatıyorsa
  `Connection: close` gönderiyor ve gerçekten kapatıyor. İstemci hiçbir durumda yanıt beklerken asılı kalmıyor.

### 3.7 CGI

#### CGI-01 — Uzantıya göre en az bir CGI — ZORUNLU
- **Kaynak:** S: "Execution of CGI, based on file extension (for example .php)", "Your server should support at least
  one CGI".
- **Kontrol:** config'teki CGI uzantısıyla biten bir dosya iste; aynı içerikli ama farklı uzantılı bir dosya iste.
- **Geçme:** ilki çalıştırılıyor, ikincisi statik dosya olarak dönüyor.

#### CGI-02 — GET ve POST — BÖLÜM (Check CGI)
- **Kaynak:** E2: "You must test the CGI with the "GET" and "POST" methods."
- **Kontrol:** `GET /cgi/x.py?a=1&b=2` → script `QUERY_STRING`'i yazdırır. `POST` ile 1 MB body → script stdin'i
  okuyup uzunluğunu ve hash'ini yazdırır.
- **Geçme:** query string birebir, body bayt bayt ulaşıyor.

#### CGI-03 — Ortam değişkenleri tam istek bilgisini taşır — ZORUNLU
- **Kaynak:** S: "Have a careful look at the environment variables involved in the web server-CGI communication. The
  full request and arguments provided by the client must be available to the CGI."
- **Kontrol:** tüm ortamı yazdıran geçici bir script; `X-Deneme: 42` header'lı bir POST isteği.
- **Geçme:** en az şunlar doğru değerle var: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`,
  `SCRIPT_NAME`, `PATH_INFO`, `SERVER_PROTOCOL`, `SERVER_NAME`, `SERVER_PORT`, `GATEWAY_INTERFACE=CGI/1.1`,
  `REMOTE_ADDR` ve her istek header'ı için `HTTP_*` (`HTTP_HOST`, `HTTP_X_DENEME=42`...).

#### CGI-04 — Body sonu EOF — ZORUNLU
- **Kaynak:** S: "the CGI will expect EOF as the end of the body."
- **Kontrol:** stdin'i EOF'a kadar okuyan (`CONTENT_LENGTH`'e bakmayan) bir script'e normal ve chunked POST.
- **Geçme:** script her iki durumda da sonlanıyor ve body'nin tamamını görüyor.

#### CGI-05 — CGI çıktısının sonu EOF — ZORUNLU
- **Kaynak:** S: "If no content_length is returned from the CGI, EOF will mark the end of the returned data."
- **Kontrol:** `Content-Length` yazmadan, parça parça ve aralarda bekleyerek 5 MB çıktı üreten script.
- **Geçme:** istemci 5 MB'ın tamamını alıyor; yanıt geçerli HTTP (status satırı + header + gövde).

#### CGI-06 — Doğru çalışma dizini — BÖLÜM (Check CGI)
- **Kaynak:** S/E2: "The CGI should be run in the correct directory for relative path file access."
- **Kontrol:** yanındaki dosyayı göreli yolla okuyan script (repodaki `html/cgi/cwd.py` + `data.txt` gibi).
- **Geçme:** dosya okunuyor, içerik yanıtta.

#### CGI-07 — Hatalı CGI'lar — BÖLÜM (Check CGI)
- **Kaynak:** E2: "You must test with CGI files containing errors to ensure that server's error handling works
  correctly. You can use a script containing an infinite loop or an error", "The server should never crash, and an
  error should be displayed if an issue occurs."
- **Kontrol:**
  - sonsuz döngü (SIGTERM'i yok sayan): beklerken başka bir istemciden `GET /` 1 sn altında dönmeli
  - sözdizimi hatası, çıkış kodu 1, hiç çıktı yok, geçersiz header satırı
  - her testten sonra `ps -o pid,stat,cmd --ppid $PID`
- **Geçme:** sonsuz döngü T + 5 sn içinde 504, süreç öldürülmüş; diğer hatalar H-03'teki 5xx; zombi (`Z`) veya
  artık CGI süreci yok; sunucu yaşıyor.

#### CGI-08 — CGI event loop'u kilitlemez — SIFIR
- **Kaynak:** S/E2 (IO-03, IO-08). E2: "CGI pipes/FIFOs must occur only when readiness is indicated".
- **Kontrol:** 10 MB body'yi tamamını okuyarak işleyen script'e POST; 10 MB çıktı üreten script'e GET; ikisi
  sürerken başka istemciden `GET /`.
- **Geçme:** veriler bütün, diğer istemci 1 sn altında yanıt alıyor (64 KB'lık pipe tamponunu aşan veri ancak
  poll ile sürülen yazma/okumayla geçebilir).

### 3.8 Port sorunları (P) — BÖLÜM (Port issues)

#### P-01 — Farklı interface/port, farklı site, tarayıcıda
- **Kaynak:** E2: "set up multiple interfaces and ports to provide different websites. Use the browser to verify".
- **Kontrol:** C-03 config'i; her adresi tarayıcıda (veya otomasyonla) aç.
- **Geçme:** her adres doğru siteyi gösteriyor.

#### P-02 — Aynı interface:port tekrarı
- C-04 ile aynı test; sonucu buraya da yaz.

#### P-03 — Ortak portlu iki webserv süreci
- **Kaynak:** E2: "Launch multiple webserv programs at the same time with different configuration files but with
  common interface:ports. Does it work? If it does, ask why. Ensure that ... the program behaviour is coherent and
  does not crash."
- **Kontrol:** ilk süreç `127.0.0.1:A` ile çalışırken ikinciyi `127.0.0.1:A` + `127.0.0.1:C` içeren config ile başlat.
- **Geçme:** ikinci süreç çöküşsüz; ya bind hatasını yazıp çıkıyor ya da hatayı yazıp kalan portlarla devam ediyor
  (hangisi olduğunu raporla). İlk süreç kesintisiz çalışıyor. İkisi de aynı portu dinleyebiliyorsa nedenini
  (`SO_REUSEPORT` vb.) koddan göster.

### 3.9 Siege ve stres (S)

Hazırlık: geçici kök dizinde 0 baytlık `empty.html`. `siege` yoksa kur (`apt-get install siege` / `dnf install
siege`). Kurulamıyorsa S-01..S-04 BELİRSİZ.

#### S-01 — Erişilebilirlik > %99.5 — BÖLÜM (Siege & stress test)
- **Kaynak:** E1/E2: "Availability should be above 99.5% for a simple GET request on an empty page using a siege with
  the -b flag." E2: "-c, -d, -r ... choice of these parameters is at the evaluator's discretion".
- **Kontrol:** `siege -b -c 50 -t 1M http://127.0.0.1:PORT/empty.html`; ardından `siege -b -c 100 -r 200 ...`.
  Kullanılan parametreleri raporla.
- **Geçme:** her koşuda `Availability` > 99.50 %.

#### S-02 — Bellek sınırsız büyümez — BÖLÜM
- **Kaynak:** E1/E2: "Monitor the process memory usage; it should not increase indefinitely".
- **Kontrol:** S-01'in ilk komutunu art arda 3 kez çalıştır; her koşu sonunda `ps -o rss= -p $PID`.
- **Geçme:** 2. ve 3. koşu sonu RSS farkı ≤ 1 MB; koşudan koşuya tekdüze artış yok.

#### S-03 — Asılı bağlantı yok — BÖLÜM
- **Kaynak:** E1/E2: "Check that there are no hanging connections."
- **Kontrol:** siege bittikten T + 5 sn sonra `ss -tan "( sport = :PORT )"` ve `ls /proc/$PID/fd | wc -l`.
- **Geçme:** `ESTAB` veya `CLOSE-WAIT` durumunda bağlantı yok; fd sayısı siege öncesi tabana eşit.

#### S-04 — Yeniden başlatmadan süresiz siege — BÖLÜM
- **Kaynak:** E1/E2: "You should be able to use siege indefinitely without having to restart the server".
- **Kontrol:** `siege -b -c 50 -t 10M ...`; ardından normal `GET /`.
- **Geçme:** availability > %99.5, sunucu yeniden başlatılmadan 200 dönüyor, S-02 ve S-03 koşulları hâlâ sağlanıyor.

#### S-05 — Bellek sızıntısı yok — SIFIR (Leaks)
- **Kaynak:** E1/E2: "Any memory allocated on the heap must be properly freed before execution ends ... valgrind".
- **Kontrol:** `valgrind --leak-check=full --show-leak-kinds=all ./webserv <conf>`; H-01..H-07, CGI-02, CGI-07
  senaryolarını ve yarıda kesilen bağlantıları çalıştır; `kill -INT` ile durdur. Aynısını geçersiz config (C-02) ve
  port çakışması (P-03) çıkış yolları için de yap.
- **Geçme:** `definitely lost`, `indirectly lost`, `possibly lost` = 0. `still reachable` kayıtlarının hiçbirinin
  yığın izinde proje kaynak dosyası yok (yalnızca libc/libstdc++ iç havuzları kabul).

### 3.10 README (D) — SIFIR (E2: "If any of the required elements is missing, the grade is 0.")

#### D-01 — Repo kökünde README.md var
#### D-02 — İlk satır italik ve birebir
- `*This project has been created as part of the 42 curriculum by <login1>[, <login2>[, <login3>[...]]].*`
  (`*` veya `_` ile italik). Kontrol: `head -1 README.md`.
#### D-03 — "Description" bölümü: projenin amacı ve kısa genel bakış
#### D-04 — "Instructions" bölümü: derleme, kurulum ve/veya çalıştırma
#### D-05 — "Resources" bölümü: klasik kaynaklar (doküman, makale, tutorial) **ve** AI'ın hangi görevlerde ve projenin hangi bölümlerinde kullanıldığı
#### D-06 — README İngilizce — ZORUNLU (S: "Your README must be written in English.")

### 3.11 Teslim dosyaları (T) — KAPSAM

Bu bölüm "teslim modu" kontrolüdür; geliştirme sırasında bulgu olarak raporlanır, teslimden önce temizlenir.

#### T-01 — Takipli her dosya izinli bir kategoride
- **Kaynak:** S: "Files to Submit: Makefile, *.{h, hpp}, *.cpp, *.tpp, *.ipp, configuration files"; README.md
  (S Ch.V); demo dosyaları (S IV.3: "default files to test and demonstrate"); test kaynakları (S IV.3).
- **Kontrol:** `git ls-files`; `git ls-files -z | xargs -0 file | grep -E 'ELF|PDF|image'`.
- **Geçme:** her dosya şu kategorilerden birinde: Makefile, kaynak/başlık, config, README.md, demo site/CGI dosyası,
  test kaynağı/script'i, `.gitignore`. İkili dosyalar (derlenmiş programlar, hazır tester binary'leri, `.o`), PDF'ler,
  demo sitesinde kullanılmayan görseller, CI tanımları, editör/derleyici yardımcı dosyaları, karalama dosyaları ve
  bu `EVAL_RULES.md` teslimde bulunmaz.

#### T-02 — Eval için gereken her şey takipli
- **Kaynak:** S: "Only the content of your repository will be evaluated". E1/E2: "check that 'git clone' is used in
  an empty folder".
- **Kontrol:** C-14'teki temiz klon testi. `.gitignore`'un demo/config için gereken bir yolu dışlayıp dışlamadığına bak.
- **Geçme:** temiz klonda her config ve her demo eksiksiz çalışıyor.

### 3.12 Bonus (B) — yalnızca tüm zorunlu kurallar GEÇTİ ise puanlanır

- **Kaynak:** S Ch.VI; E1/E2: "Evaluate the bonus part only if the mandatory part has been fully completed".

#### B-01 — Cookie ve session
- **Kontrol:** ilk istekte `Set-Cookie` geliyor; aynı cookie ile ikinci istek oturum durumunu koruyor (ör. sayaç
  artıyor); farklı istemci farklı oturum alıyor. Repoda basit bir örnek var (S: "provide simple examples").
#### B-02 — Birden fazla CGI türü
- **Kontrol:** en az iki farklı CGI sistemi (ör. `.py` ve `.php`) CGI-02..CGI-07'yi ayrı ayrı geçiyor.

### 3.13 İnsan kontrolü (Q) — ajan UYGULANAMAZ yazar, ama hazırlık çıktısını üretir

#### Q-01 — Savunma soruları (E1/E2 "Check the code and ask questions")
HTTP sunucusunun temeli; hangi olay mekanizması (poll/select/epoll/kqueue); nasıl çalıştığı; tek çağrıyla accept,
okuma ve yazmanın nasıl yönetildiği; poll'dan istemci I/O'suna giden kod yolu.
**Ajan çıktısı:** IO-03'teki I/O yol haritası ve ana döngünün `dosya:satır` özeti.
#### Q-02 — Kodun anlaşılması ve küçük değişiklik
S AI Instructions: "Only use AI-generated content that you fully understand". S Ch.VII: savunmada küçük bir
değişiklik istenebilir. Ekip her satırı açıklayabilmeli.

---

## 4. Test ortamı

- **Araçlar:** `make`, `c++`, `curl`, `nc` (veya `telnet`), `siege`, `valgrind`, `python3`, `ss`, `ps`, `prlimit`, `file`, `nm`, `ldd`.
  Eksik araç → ilgili kural BELİRSİZ.
- **Derleme:** `make re`.
- **Geçici kök:** `D=$(mktemp -d)`; altında `a/`, `b/`, `c/` (farklı `index.html`), `www/pouic/toto/pouet`,
  `liste_acik/`, `liste_kapali/` (index'siz), `yukle/`, `cgi/` (test script'leri), `empty.html` (0 bayt),
  `buyuk` (200 MB), `kucuk.html`.
- **Geçici config:** direktif adları projenin parser'ından alınır (K-04 madde 1). Adlar kural değildir; davranış
  kuraldır. Config'teki tüm yollar `$D` altındaki mutlak yollar olmalı. Body limiti testleri (C-07) ayrı bir server
  bloğunda yapılır ki upload testleri etkilenmesin.
- **Sırası:** G → F → IO (statik inceleme) → C → H → CGI → R → P → S → D → T → B → K-04 envanteri.
- **Otomatik testler:** `tests/eval/static_checks.py` (G, F, IO; `--teslim` ile D, T, K-03) ve `tests/eval/e2e.py`
  (varsayılan: C, H, CGI, R-01, IO-07/08, P-03; `--heavy`: R-02, IO-09, CGI-08; `--siege`: S-01..S-04;
  `--valgrind`: S-05; `--bonus`: B-01, B-02; `--nc`: H, R, C-03 ham istekler; `--nc-valgrind`: aynisi valgrind altinda + S-05). Her çıktı satırı kural ID'si taşır; GitHub Actions'ta `.github/workflows/ci.yml` ve
  `teslim.yml` bunları koşar. Bu çıktılar kanıt olarak kullanılabilir ama denetimin yerini tutmaz: IO-02/03/04/06,
  H-04 (tarayıcı), K-01/K-04, D-06 ve Q elle incelenir.

---

## 5. Rapor şablonu

```
# Denetim Raporu — <tarih> — <git rev-parse --short HEAD>

Nihai karar: UYUMLU | UYUMSUZ
SIFIR KALDI: n · BÖLÜM KALDI: n · ZORUNLU KALDI: n · KAPSAM: n · BELİRSİZ: n

## Kurallar
| ID | Ağırlık | Sonuç | Kanıt |
|----|---------|-------|-------|
| G-01 | SIFIR | GEÇTİ | `make re` → hatasız, uyarısız |
| ...  |       |       |       |

## I/O yol haritası (IO-03 kanıtı)
| Çağrı (dosya:satır) | fd türü | Tetikleyen olay | poll'dan gelen yol |

## Kapsam envanteri (K-04)
| Özellik / dosya | Konum | Eşlendiği kural | Durum (izinli / bulgu) |

## Kaynak çelişkileri ve yorum farkları
## Riskler (değerlendiriciye göre değişebilecek noktalar)
```
