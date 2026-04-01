* **Bu Dosyanın Amacı:** Bu dosya benim yaptığım projede şuan hangi aşamada olduğumu hangi projeyi yaptığı proje bilgilerini ve bilip bilmediğim şeyleri anlatıyor. Senin görevin bu dosyayı okuyup ben öğrendikçe ve projeyi geliştirdikçe bu dosyayı güncellemen. pdf dosyayını okumayı unutma main.cyi de oku 

# 🚀 Proje Durumu: 42 Webserv

## 📌 Genel Bilgiler
* **Proje Adı:** Webserv 
* **Amaç:** C++98 kullanarak, HTTP protokolünü anlayan kendi web sunucumuzu (HTTP server) yazmak[cite: 4, 90, 91].
* **Referans Kaynak:** `PDF/en.subject.pdf` (Projenin kuralları ve bütün temel bilgileri).
* **Mevcut Aşama:** İlk çalışan bloklayıcı (blocking) prototip yazıldı ve HTTP iletişimi tarayıcılarla test edildi. Şu an projeyi kurallara uygun hale getirecek olan Non-Blocking (Bloklanmayan) mimariye ve konfigürasyon (config) okuma aşamasına geçiş yapılıyor.

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

### 7. CGI (Common Gateway Interface) Mekanizması ve Python/PHP Şartı
* Webserv sadece `.html`, `.css` gibi statik dosyaları okuyup yollamakla kalmaz.
* PDF'ye göre **"en az bir tane"** dinamik çalıştırılabilir dil (interpreter) desteklemek zorundasın (Python, PHP vb.).
* Eğer tarayıcıdan `GET /script.py HTTP/1.1` veya `POST /gonder.php HTTP/1.1` diye bir istek gelirse, bunu C++'ın okuyup kendisi döndürmez.
* **Nasıl Çalışır? (CGI Algoritması)**
  1. `fork()` ile programı ikiye böl (Child Process oluşur).
  2. Child Process, `execve()` kullanarak işletim sistemindeki Python'u (veya PHP'yi) çağırır ve ona o `.py` (veya `.php`) dosyasını çalıştır der.
  3. C++ Webserv'in (Parent Process) Python'un ürettiği çıktıyı okuyabilmesi için `pipe()` kullanılır.
  4. Script (Python/PHP) işini bitirip çıktıyı (örn: `<h1>Giriş Başarılı</h1>`) pipe içine tükürür.
  5. Parent process pipe'dan bu çıktıyı okuyup tarayıcıya (istenilen HTTP formatında) gönderir.
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

### 6- Kapanış
* İşi biten veya bağlantıyı koparan client'ı `close()` ile kapat ve `poll` vektöründen temizle.

---

## 🎯 Proje Felsefesi ve Mimari Farkındalık: Web Sunucusu vs Backend
* **Aydınlanma Anı:** "Aslında webserver'dan bağımsız bir backend yazacağım." tespiti %100 doğru bir bakış açısıdır.
* **C++ Webserv (Web Sunucusu Katmanı - Postacı/Kapıcı):** Senin projenin ana omurgasıdır. TCP ağ protokolünü, soketleri, multiplexing'i (`poll`) işler. HTTP formatını ayrıştırır. Ancak HTTP'nin barındırdığı bilginin "mantığıyla/işiyle" ilgilenmez. Sadece iletimden, dağıtımdan ve çoklu bağlanan müşterilere (client'lara) anında cevap verilmesinden sorumludur. Nginx veya Apache gibi çalışır.
* **Python/PHP vb. Dosyalar (Backend/CGI Katmanı - Aşçı):** Kullanıcının veritabanına kaydedilmesi, şifre kontrolü veya hesaplama gibi mantıksal işleri yapan *bağımsız* kod parçalarıdır. Python veya PHP scriptleri HTTP raw (çiğ) protokolünü veya TCP soketlerini asla bilmez. Sadece standart girdi (`stdin`) okuyup, standart çıktı (`stdout`) üretir.
* **Sonuç:** Senin C++ Web Sunucun, dışarıdan gelen müşteriyi (Browser) alacak ve ona ne istediğini soracak. Eğer Müşteri "Bana sadece resim ver" (GET) derse depodan resmi alıp verecek. Eğer Müşteri "Bu form verisini al, şifremi kontrol et" (POST) derse, C++ sunucun bunu o *bağımsız arkadaki backend scriptine (Python)* paslayacak (`execve` ve `pipe` ile), onun cevabını alıp müşteriye (Browser'a) kargo edecektir. Kendi yazdığımız web sunucumuz bir nevi "Trafik Polisi" dir. Müşteri (Browser) asla o arkaplandaki Python (backend) dosyasını doğrudan göremez veya ulaşamaz.