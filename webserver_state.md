* **Bu Dosyanın Amacı:** Bu dosya benim yaptığım projede şuan hangi aşamada olduğumu hangi projeyi yaptığı proje bilgilerini ve bilip bilmediğim şeyleri anlatıyor. Senin görevin bu dosyayı okuyup ben öğrendikçe ve projeyi geliştirdikçe bu dosyayı güncellemen. Pdf dosyayını oku dizinini "Referans Kaynak"ta belittim.

# 🚀 Proje Durumu: 42 Webserv

## 📌 Genel Bilgiler
* **Proje Adı:** Webserv 
* **Amaç:** C++98 kullanarak, HTTP protokolünü anlayan kendi web sunucumuzu (HTTP server) yazmak
* **Referans Kaynak:** `PDF/en.subject.pdf` (Projenin kuralları ve bütün temel bilgileri).
* **Mevcut Aşama:** İlk çalışan bloklayıcı (blocking) prototip yazıldı ve HTTP iletişimi tarayıcılarla test edildi. Şu an projeyi kurallara uygun hale getirecek olan Non-Blocking (Bloklanmayan) mimariye ve konfigürasyon (config) okuma aşamasına geçiş yapılıyor. (PDF `subject.txt` dosyası okunarak gereksinimler teyit edildi).

---

## ✅ Şu Ana Kadar Öğrenilenler (Kavranan Teorik/Teknik Konular)

### 1. Soket Programlama Temelleri
* **Socket Oluşturma (`socket()`):** İletişim için bir uç nokta (endpoint) veya telefon ahizesi oluştururuz.
  * `AF_INET`: IPv4 protokolünü kullanır.
  * `SOCK_STREAM`: TCP üzerinden güvenilir veri iletimi sağlar.
* **Adresleme ve Bağlama (`bind()`):** Sokete (fd) spesifik bir IP (`INADDR_ANY` ile hepsi) ve Port numarası (örn: 8080) atar. Portlar için byte sıralaması (`htons()`) kullanılır.
* **Kapıları Açma (`listen()`):** Soketi pasif/dinleyici moda geçirir. İşletim sistemine, gelen bağlantıları bekletmesi için bir bekleme odası (backlog kuyruğu) ayırmasını söyler.
* **Müşteriyi İçeri Alma (`accept()`):** Kuyruktaki ilk bağlantı isteğini kabul eder. Orijinal dinleyici soket kapıda beklemeye devam ederken, `accept()` bize sırf o istemciyle konuşmamız için **yepyeni bir dosya tanımlayıcısı (client_socket)** verir.

### 2. HTTP Protokolü İletişimi
* **İstekler (Request):** Tarayıcıların bize gönderdiği metinler. `GET / HTTP/1.1` ile başlar, başlıklar (headers) içerir ve `\r\n\r\n` (boş satır) ile biter.
* **Yanıtlar (Response):** Bizim tarayıcıya döndüğümüz yapı. 
  * Durum satırı (`HTTP/1.1 200 OK`)
  * Başlıklar (`Content-Type: text/html`, `Content-Length: ...`)
  * Sihirli boş satır (`\r\n\r\n`)
  * Gövde (C++ `ifstream` ile okuduğumuz `index.html` dosyası vb.).

### 3. Port Kavramı ve Yönetimi
* **Portlar:** 0 ile 65535 arasında 16-bitlik numaralardır.
  * `0-1023`: Sistem portlarıdır (root/sudo yetkisi ister).
  * `1024-49151`: Uygulamalarımız için ideal (Örn: 8080, 9000).
* **Port Çakışması:** Aynı makinede, aynı portu dinleyen iki uygulama olamaz (`EADDRINUSE` hatası alınır).

### 4. İşletim Sistemi Ağ Komutları
Aktif olarak dinlenen portları ve ağ durumunu kontrol etmek için kullandığım araçlar:
* **Ubuntu (Linux) Tarafı:** `ss -tuln` (Dinlenen portları hızlıca listeler) veya `lsof -i -P -n` (Portu hangi uygulamanın tuttuğunu gösterir).
* **Windows 10 Tarafı:** `netstat -ano` (Aktif portları ve süreç ID'lerini (PID) listeler).

### 5. std::vector Kullanımı ve Birden Fazla Port Dinleme
* Normal C dizileri yerine C++'ın esnek `std::vector` yapısını kullanmak.
* Config okundukça her bir `server {}` bloğu için ayrı bir `socket()`, `bind()`, `listen()` yapıp, elde edilen file descriptor'ları (FD) bir vektörde (örneğin: `std::vector<int> server_sockets;`) depolamak.

### 6. Gerçek Dünya (Nginx) vs CGI Projemiz Farkı (Teyit Notu)
* **Gerçek Dünyada Modern Sistemler (FastCGI, uWSGI, Gunicorn):** Nginx ve Python (veya Node.js) her ikisi de 7/24 arkada **ölmeden** ayrı ayrı çalışır. Nginx asıl web portunu (80) dinler. Python ise arka portta (örn. 8000) kendi özel dünyasıyla bekler. Nginx 80'den HTTP alınca, arka plandaki Python'a (8000. porta) onu tcp/socket/unix-socket üzerinden fırlatıp paslaşırlar. Python ASLA KAPANMAZ!
* **Bizim 42 Webserv (CGI - Common Gateway Interface) Projemizde:** C++ Webserv'imiz 7/24, 80/8080. portta çalışır. Python ise **KAPALIDIR**. Ne zaman ki istek (GET /test.py) gelirse, asıl sunucumuz C++ `fork() + execve()` yapıp Python'u anlık "diriltir" (uyandırır). Python işini yapıp `stdout/pipe` içine tükürür ve anında ölür. Modern siteler saniyede binlerce istek için sürekli uyanık tutarken, tarihsel ama yapması daha "zor/öğretici" CGI modeli sadece çağrılınca çalış-bitir-öl sistemidir.
* **🚨 Kritik Yanılgı Düzeltmesi (Veriyi Müşteriye Kim Yolluyor?):** Python kullanıcıya (Chrome/Browser'a) ASLA doğrudan veri yollamaz! Python'un internetten, TCP soketinden, ağ bağlantısından haberi bile yoktur. Python sadece `print()` ile ekrana yazı yazar gibi kapalı bir `pipe` (boru) içine metin yazar. C++ (Webserv) o borunun diğer ucundan Python'un ürettiği raw (çiğ) HTML'i okur, önüne `HTTP/1.1 200 OK` gibi resmi HTTP başlıklarını (kargo zarfını) ekler ve işlemi bitmiş, çoktan ölmüş olan Python'un "mirasını" alıp müşteriye (Sokete) **bizzat C++ programı** gönderir. İletişim yüzü daima C++'tır.
  3. `fork()` ile programı ikiye böl (Child Process oluşur). 
     *(Neden fork? Çünkü execve programı tamamen değiştirip sonlandırır. Asıl sunucu (Parent) yaşamaya ve port dinlemeye devam etmelidir. Klon (Child) kendini feda eder.)*
  4. Child Process, `execve()` kullanarak işletim sistemindeki Python'u çağırır ve ona o `.py` dosyasını çalıştır der.
  5. C++ Webserv'in (Parent Process) Python'un ürettiği çıktıyı okuyabilmesi için `pipe()` kullanılır.
  6. Script (Python) işini bitirip çıktıyı (örn: `<h1>Giriş Başarılı</h1>`) pipe içine tükürür ve ölür.
  7. Parent process pipe'dan bu çıktıyı okuyup tarayıcıya (istenilen HTTP formatında) gönderir.
* **Sonuç:** Kodun %99'u C++98 olacak. Ama test edebilmek ve bu (CGI) kuralı yerine getirebilmek için yanına 1-2 tane örnek `.py` (veya `.php`) dosyası yazmamız (veya hazır bulmamız) gerekecek. Python/PHP webe özel arka planda çalışan o küçük scriptlerdir, asıl sunucumuz kesinlikle C++ olacak.
* **Neden İhtiyacımız Var?** Yazdığımız ilk kod `accept` ve `read` işlemlerinde bloklanıyor (kilitleniyor). PDF kuralları gereği sunucu asla kilitlenmemelidir.
* **Çözüm:** `poll()` fonksiyonu ile işletim sistemine bir `std::vector<struct pollfd>` listesi vererek "Olay Döngüsü (Event Loop)" kurmak.
* **Kural:** İstemciler ve sunucu arasındaki tüm I/O işlemleri, okuma ve yazmayı aynı anda izleyen tek bir `poll()` (veya eşdeğeri) üzerinden yönetilmelidir. İşletim sistemi `POLLIN` veya `POLLOUT` ile bize haber vermeden asla `read` veya `write` yapamayız.

---

## ✅ Tamamlananlar (Pratik Kodlama)

* **Temel Soket Kurulumu:** `socket()` ile soket oluşturma ve `bind()` ile bir porta bağlama.
* **İlk Çalışan Sunucu (main.cpp):** `listen` ve sonsuz döngü içinde `accept` kullanarak temel bir sunucu ayağa kaldırıldı.
* **Statik Dosya Sunumu:** İstemciden gelen istek karşılanıp, C++ disk işlemleri (`std::ifstream`) kullanılarak yerel bir `index.html` dosyası okundu ve tarayıcıya başarıyla HTTP yanıtı olarak gönderildi.
* **Testler:** Kod hem `telnet localhost 8080` ile manuel olarak, hem de Google Chrome üzerinden gerçek dünya senaryosuyla test edildi.

---

## 🚧 Güncel Yol Haritası ve Mimari Algoritma (To-Do List)

### 1- Config ve Hazırlık Aşaması
* **Config Dosyası Okuma (Parser):** `webserv.conf` dosyasını parse et.
* **Server Sayısını Belirleme:** Parser'dan kaç tane server (farklı port/IP) açılacağını öğren.
* **Soket Vektörleri:** Her server için bir TCP Socket oluştur ve File Descriptor'ları (FD) bir `std::vector<int>` içinde tut.

### 2- Server Başlatma (Init)
* Her bir server soketi için:
  * `bind()` ile IP ve Port ata.
  * `listen()` ile dinlemeye başla.
  * **KRİTİK:** `fcntl()` kullanarak tüm server soketlerini `O_NONBLOCK` (bloklanmayan) moda geçir.

### 3- Event Loop (Olay Döngüsü) ve `poll()` Motoru
* Tüm server soketlerini ve (daha sonra eklenecek olan) client soketlerini takip etmek için bir `std::vector<struct pollfd>` oluştur.
* Sonsuz bir `while(true)` döngüsü başlat ve içinde `poll()` çağır. `poll()`, işletim sistemine "Bu soketlerde bir hareket (yeni bağlantı veya yeni mesaj) olana kadar bekle" der.

### 4- Gelen İstekleri Yönetme (Accept & Read)
* **Yeni Müşteri (Server Soketi Çaldıysa):** `accept()` ile müşteriyi al. Yeni oluşan client soketini de `O_NONBLOCK` yap ve `poll`'un izlediği listeye (`pollfd` vektörüne) ekle.
* **Tarayıcıdan Mesaj (Client Soketi Çaldıysa):** `read()`/`recv()` ile gelen HTTP parse işlemi için oku ve bir Buffer'a yaz.

### 5- HTTP Request Parse ve İşlem (GET, POST, DELETE)
Okunan veriden Metod (GET/POST/DELETE), Hedef URL ve Body (Gövde) ayrıştırılır:
* **GET:** İstenen HTML/görsel dosyasını diskten oku, HTTP başlıkları (200 OK) ile `write()`/`send()` kullanarak yolla.
* **POST:** Tarayıcıdan gelen form verisini veya yüklenen (upload) dosyayı diske kaydet (201 Created).
* **DELETE:** İstenen dosyanın varlığını kontrol et ve `remove()` ile diskten sil (204 No Content).

### 5.5- HTTP Yanıt Kodları (Status Codes) ve Hata Yönetimi
Projeye entegre edilecek geri dönüş (HTTP Status) kodları ve kullanılacakları yerler (Yanıt/Response oluşturulurken entegre edilir):
* **2xx Başarılı (Success):** 
  * `200 OK`: İstek eksiksiz alındı (GET) ve cevap hazır.
  * `201 Created`: İstemci başarıyla dosya yükledi (POST upload).
  * `204 No Content`: Dosya başarıyla silindi (DELETE) ve gövde (body) döndürülmeyecek.
* **3xx Yönlendirme (Redirection):**
  * `301 / 302 / 307`: `webserv.conf` dosyasında bir yol (route) için HTTP yönlendirme (redirection) kuralı eklenmişse kullanılır.
* **4xx İstemci Hataları (Client Error):**
  * `400 Bad Request`: Gelen HTTP isteği hatalı/bozuk format ile parse edilemiyorsa.
  * `403 Forbidden`: Dosyanın okuma yetkileri kapalıysa veya klasör istenirken `directory listing` (autoindex) erişimi conf'ta kapalıysa.
  * `404 Not Found`: İstenen HTML, medya dosyası veya CGI scripti dizinde bulunamadıysa.
  * `405 Method Not Allowed`: İstek yapılan Rota (URL) için Config'de tanımlanan spesifik bir izin (örneğin sadece GET izni var) varsa ve farklı bir method (örn: POST) ile gelindiyse.
  * `413 Payload Too Large`: İstemcinin yolladığı istek gövdesi (Body), Config'teki limitleri (max_body_size) aşarsa.
* **5xx Sunucu Hataları (Server Error):**
  * `500 Internal Server Error`: CGI (Python/PHP vs) çalışması sırasında bir çökme olursa, ya da `fork()/execve()` hatalarında.
  * `501 Not Implemented`: Sunucunun desteklemediği bir HTTP metodu (PUT, PATCH, OPTIONS vb.) kullanıldıysa.

👉 **Nasıl Entegre Edilecek?** 
İsteği (Request) ayrıştırırken (Parse ederken) kurallara aykırı durumlarda fonksiyon akışı hemen Response (Yanıt) oluşturma moduna geçer ve ilgili **Hata Kodu** belirlenir. Ardından:
1. `webserv.conf` dosyasında bu kod için atanmış özel bir dosya (örn: `error_page 404 /ozel_404.html`) var mı diye sorgulanır, varsa o HTML dosyası okunur.
2. Özel dosya config'de yoksa veya okunamıyorsa, C++ kodumuz içinde string olarak ("Hardcoded") sakladığımız basit ve default **gömülü hata sayfası HTML'i** oluşturulup HTTP Başlıklarıyla (`HTTP/1.1 404 Not Found\r\n...`) birlikte tarayıcıya yollanır.

### 6- Kapanış
* İşi biten veya bağlantıyı koparan client'ı `close()` ile kapat ve `poll` vektöründen temizle.

---

## 🎯 Proje Felsefesi ve Mimari Farkındalık: Web Sunucusu vs Backend (CGI)
* **Aydınlanma Anı:** "C++ ile bağımsız bir webserver (altyapı) yazıyoruz. İşin mantığını (backend) ise ayrı süreçlerde hallediyoruz."
* **C++ Webserv (Web Sunucu Katmanı):** Projenin omurgasıdır. TCP protokolünü, soketleri, `poll()` ile multiplexing işlemlerini yönetir. HTTP requestlerini ayrıştırır (parse eder), client'ları dinler ve client'lara veri yollar.
* **CGI Scriptleri (Backend Katmanı - Python/PHP):** TCP veya Ağ soketlerinden tamamen habersizdir. HTTP protokolünü kendisi çözmez (bunu Web Sunucusu yapıp ona çevre değişkenleri olarak iletir). Sadece "standart girdi" (`stdin`) okuyup, veritabanına kayıt gibi mantıksal işlemleri yapar ve "standart çıktı" (`stdout`) üretir.

### Teknik Olarak CGI ve C++ Haberleşme Adımları (Müşteriye Veri Ulaşana Kadar)
Browser (Google Chrome), sunucuya bir POST veya dinamik bir GET isteği (örneğin `/script.py`) attığında şu olaylar gerçekleşir:
1. **İsteği Karşılama:** C++, TCP Socket üzerinden request'i `read()`/`recv()` ile alır ve içeriğini parse ederek CGI çalıştırılması gerektiğini (dosya uzantısından) anlar.
2. **Pipe (Boru) Kurulumu:** C++, Python'un çıktısını okuyabilmek için işletim sisteminden 2 uçlu bir haberleşme kanalı açar: `pipe(fd_array)`.
3. **Kopya Oluşturma (Fork):** C++ programı `fork()` sistem çağrısı ile kendini kopyalar (Bir Parent, bir Child process oluşur). *Asıl sunucu (Parent) dinlemeye ve diğer client'lara hizmet vermeye devam etmelidir.*
4. **Çıktı Yönlendirme (Dup2):** Child (kopya) process, `dup2(fd_array[1], STDOUT_FILENO)` diyerek *Standart Çıktısını (Yani terminale bastığı yazıları)* oluşturduğumuz Pipe'ın (borunun) yazma ucuna bağlar.
5. **Python'u Tetikleme (Execve):** Child process, `execve("/usr/bin/python3", "script.py", ortam_degiskenleri)` diyerek kendi bedenini Python script'ine dönüştürür.
6. **İşlemsel Süreç & Çıktı (Python):** Python işini yapar ve C++'ın ona dup2 ile bağladığı için haberi bile olmadan Pipe kanalına `print("<h1>Başarılı</h1>")` şeklinde çıktısını (raw HTML) yollar. Ve process sonlanır (ölür).
### 7- HTTP ve Web Sunucularında "Encoding" (Kodlama) Kavramı
**Encoding (Kodlama)**, bilginin bir sistemden diğerine aktarılırken, o sistemin anlayabileceği, taşıyabileceği veya güvenli bulduğu bir formata (genellikle byte dizisine) dönüştürülmesidir. 42 Webserv projesinde karşına çıkacak **3 temel Encoding** türü vardır:

#### 1. Karakter Kodlaması (Character Encoding - UTF-8, ASCII)
* Bilgisayarlar sadece 0 ve 1'leri anlar. "A" harfini veya "Ç" harfini 0 ve 1'lere çevirme kurallarına karakter kodlaması denir.
* HTTP başlıkları (Headers) her zaman **ASCII** standartlarındadır (`\r\n` ile ayrılır).
* Yanıt gövdesinde (Body) bir HTML gönderirken genelde **UTF-8** kullanırız. Tarayıcının bunu doğru okuması için şu başlığı yollarız:
  `Content-Type: text/html; charset=UTF-8`

#### 2. URL Kodlaması (URL / Percent Encoding)
* İnternet tarayıcıları hedefe (URL'ye) giderken boşluk veya özel karakter (Türkçe harfler, vb.) kullanamazlar. Bunları `%` işareti ve 16'lık (hex) sayı sistemine dönüştürürler.
* Örneğin; tarayıcıdan `http://localhost:8080/benim dosyam.html` istenirken, tarayıcı sunucuya şunu yollar: `GET /benim%20dosyam.html HTTP/1.1`
* **Projedeki Görevin:** C++ sunucun `/benim%20dosyam.html`'i gördüğünde, diske gidip dosyayı bulabilmek için önce `%20`'yi tekrar boşluğa çevirmelidir (URL Decoding).

#### 3. Transfer Kodlaması (Transfer-Encoding: chunked) ⚠️ PDF'TE İSTENEN ZORUNLULUK
* Normalde tarayıcıya veya sunucuya bir veri yollarken `Content-Length: 1024` diyerek verinin toplam boyutunu baştan bildiririz.
* Ancak bazen CGI (Python/PHP) çok büyük bir veri üretiyorsa veya istemci sunucuya devasa bir video yüklüyorsa boyutu *baştan bilemeyiz*.
* Bu durumda veri **parça parça (chunked)** gönderilir. PDF kurallarında şu yazar: *"for chunked requests, your server needs to un-chunk them"*.
* **İşleyişi:** Tarayıcı `Transfer-Encoding: chunked` başlığı yollar. Veri bloklar halinde gelir. Her bloğun başında o bloğun Hexadecimal (16'lık) boyutu yazar, altında veri bulunur. `0\r\n\r\n` (sıfır boyutta bir parça) geldiğinde iletimin bittiği anlaşılır.
* **Projedeki Görevin:** Sunucun bu tarz (chunked) bir POST isteği aldığında, içindeki Hex boyutlarını temizleyip salt veriyi birleştirmeli (un-chunk) ve sonra diske veya CGI'a vermelidir.

---

## 🌐 Büyük Resim: Klasik Web (Bizim Projemiz) vs Modern Web ve Hosting Mimari Farkı

### 1- Neden HTML'i Baştan Aşağı Yeniden (Eşşek Gibi) Yazıyoruz? (SSR vs CSR)
* **Klasik Ağaç: Server-Side Rendering (SSR - Bizim Projemiz/1990'lar):** Kullanıcı "5+5" dediğinde veya form yolladığında; C++ sunucumuz `hello.py`'ı `fork`lar, Python 10 sonucunu bulup *en baştan* sıfır bir `<html>...<h1>10</h1>...</html>` string'i üretir. Sunucu bu kocaman metni tarayıcıya yollar. Tarayıcı ekranı beyazlatır ve her şeyi baştan yükler. (Bu projede HTML'i amele gibi satır satır Python'dan print ile basmamızın sebebi budur, saf CGI böyle çalışır).
* **Modern Ağaç: Client-Side Rendering (CSR - React / Vue / AJAX / JavaScript):** Artık her tıklamada C++ sunucusuna (backend'e) gidilmiyor. C++ sadece en başta boş bir HTML ve devasa bir `script.js` (JavaScript) dosyasını kargolar (Statik). JS, tarayıcının beyninde (Chrome V8/Kullanıcı RAM'i ve CPU'su) yaşar.
* **DOM (Canlı Ağaç) Mucizesi:** JS sayfadaki `<text id="sonuc">` gibi kısımları, sayfayı yenilemeden (DOM Manipülasyonu) sayesinde saniyede bulur ve değiştirir. Eğer veritabanı veya şifre kontrolü lazımsa, JS arkaplanda C++'tan gizlice (AJAX/Fetch ile) örneğin `{"sonuc": 10}` gibi saf "JSON" verisi ister. Bizim C++ yine Python'u çağırır, ama Python bu sefer koca HTML basmaz, sadece o minik veriyi verir. C++ bunu JS'ye yollar, JS sayfayı kendi süsler.
* **Özet:** C++ Sununumuz bir kargocudur. Kutuda koca bir HTML mi var, yoksa sadece `{"sonuc":10}` yazılı minik bir not mu var ilgilenmez. Hedefe götürür.

### 2- Hosting Firmaları (AWS, GoDaddy) Bizim Kodlarımızı Nasıl Çalıştırıyor?
* "Otomatik HTML mi üretiyorlar?" Hayır! Host firmasının sunduğu sadece 7/24 kapıda bekleyen devasa bir Trafik Polisi'dir (NGINX/Apache).
* Senin kodunun içinde 1 milyonluk döngü mü dönüyor, site basit bir sayaç mı host'un umurunda değildir. Kodu C++ (veya Python/PHP) olarak bizzat SEN YAZARSIN ve onların (Harddiskine) yüklersin.
* Biri `seninsiten.com/hesapla.py` istediğinde, NGINX sadece "Biri benden .py istedi, ben karışmam, bu kullanıcının diske attığı scripti Python'a (CGI) paslıyayım o çözsün" der.
* Bizim projemizdeki asıl amacımız tam da bu! Kendi bilgisayarımızdaki kodları arka planda Python'a fırlatacak o NGINX "Trafik Polisini" C++ ile, `socket`, `bind`, `listen`, `poll` kullanarak sıfırdan inşa ediyoruz!