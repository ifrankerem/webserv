* **Bu Dosyanın Amacı:** Bu dosya benim yaptığım projede şuan hangi aşamada olduğumu hangi projeyi yaptığı proje bilgilerini ve bilip bilmediğim şeyleri anlatıyor. Senin görevin bu dosyayı okuyup ben öğrendikçe ve projeyi geliştirdikçe bu dosyayı güncellemen. pdf dosyayını okumayı unutma main.cyi de oku 

# 🚀 Proje Durumu: 42 Webserv

## 📌 Genel Bilgiler
* **Proje Adı:** Webserv 
* [cite_start]**Amaç:** C++98 kullanarak, HTTP protokolünü anlayan kendi web sunucumuzu (HTTP server) yazmak[cite: 4, 90, 91].
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

### 5. Multiplexing ve `poll()` Motoru (Teorik Olarak Kavrandı)
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

## 🚧 Öğrenilecekler / Sıradaki Adımlar (To-Do List)

1.  **Konfigürasyon Dosyası (`.conf`) Ayrıştırma (Parsing):** *(Öncelikli Adım)*
    * NGINX benzeri bir dosyayı okuyup C++ objelerine (class/struct) çevirmek.
    * Birden çok sunucu (arayüz:port çifti) dinleme ayarlarını çekmek.
    * Route (yol) bazlı kuralları (allow_methods, kök dizin, error_pages, directory listing) değişkene aktarmak.

2.  **Bloklanmayan (Non-Blocking) Mimarisine Geçiş:**
    * Şu anki tekil `while(true)` yapısını yıkıp, `poll()` fonksiyonunu merkeze alan yeni bir Event Loop inşa etmek.
    * `fcntl()` kullanarak tüm soketleri non-blocking moda geçirmek. (MacOS için özel bayraklar: `F_SETFL, O_NONBLOCK, FD_CLOEXEC`).
    * `poll()` yapısını hem yeni bağlantıları (`accept`) hem de var olan müşterilerden gelen verileri (`read`/`recv`) aynı anda dinleyecek şekilde kurgulamak.

3.  **HTTP Request (İstek) Ayrıştırma (Parsing):**
    * Müşteriden okunan `buffer` içindeki o devasa HTTP metnini parçalamak.
    * İlk satırdan metodu (`GET`, `POST`, `DELETE`) ve hedef URL'i ayıklamak.
    * Gönderilen veri varsa `Content-Length` başlığını bulup, config dosyasındaki istemci gövde boyutu limitiyle (client_max_body_size) karşılaştırmak.

4.  **CGI (Common Gateway Interface) Entegrasyonu:**
    * Eğer istenen dosya bir `.php` veya `.py` ise, programın `fork()` yapıp bu scripti harici olarak çalıştırması.
    * Tarayıcıdan gelen bilgileri Environment Variables (Ortam Değişkenleri) olarak bu CGI scriptine paslamak.

5.  **Diğer Gereksinimler:**
    * `Makefile` oluşturma (`NAME`, `all`, `clean`, `fclean`, `re` kuralları ile).
    * Herhangi bir dış kütüphane kullanmadan tam `C++98` uyumluluğu sağlamak.