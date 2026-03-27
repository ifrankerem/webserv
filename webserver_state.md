* **Bu Dosyanın Amacı:** Bu dosya benim yaptığım projede şuan hangi aşamada olduğumu hangi projeyi yaptığı proje bilgilerini ve bilip bilmediğim şeyleri anlatıyor. Senin görevin bu dosyayı okuyup ben öğrendikçe ve projeyi geliştirdikçe bu dosyayı güncellemen. pdf dosyayını okumayı unutma main.cyi de oku

# 🚀 Proje Durumu: 42 Webserv

## 📌 Genel Bilgiler
* **Proje Adı:** Webserv 
* **Amaç:** C++98 kullanarak, HTTP protokolünü anlayan kendi web sunucumuzu (HTTP server) yazmak[cite: 4, 90, 91].
* **Referans Kaynak:** `PDF/en.subject.pdf` projnin kuralları ve geri kalan bütün bilgiler burda bunu kesin oku
* **Mevcut Aşama:** Teorik araştırma ve teknik altyapı hazırlığı (Kodlama aşamasına henüz geçilmedi).

---

## ✅ Şu Ana Kadar Öğrenilenler (Kavranan Teorik/Teknik Konular)

### 1. Soket Programlama Temelleri
* **Socket Oluşturma (`socket()`):** İletişim için bir uç nokta (endpoint) veya telefon ahizesi oluştururuz.
  * `AF_INET`: IPv4 protokolünü kullanacağımızı belirtir.
  * `SOCK_STREAM`: TCP üzerinden güvenilir ve kayıpsız veri iletimi sağlar.
  * *Not:* Soketler aslında işletim sistemi gözünde özel yetenekleri olan birer Dosya Tanımlayıcıdır (File Descriptor - fd).

### 2. Adresleme ve Bağlama (`bind()`)
* **Bind İşlemi:** Oluşturulan isimsiz sokete (fd) spesifik bir IP adresi ve Port numarası atar. 
* **Struct Kullanımı:** Ağ üzerinden veri yollarken adresleri `sockaddr_in` yapısı içinde tutarız.
* **IP Ataması (`INADDR_ANY`):** Bilgisayardaki tüm ağ arayüzlerinden (localhost, Wi-Fi IP'si vb.) gelen istekleri kabul etmek için `INADDR_ANY` (0.0.0.0) makrosunu kullanırız. Özel bir IP vermek için `inet_addr("127.0.0.1")` kullanılır.
* **Byte Sıralaması (`htons()`):** Port numarası gibi verileri bilgisayarın bellek dizilişinden (Host), Ağ dizilişine (Network) çevirmek zorundayız.

### 3. Port Kavramı ve Yönetimi
* **Portlar:** 0 ile 65535 arasında 16-bitlik numaralardır.
  * `0-1023`: Sistem portlarıdır (root/sudo yetkisi ister).
  * `1024-49151`: Uygulamalarımız için ideal (Örn: 8080, 9000).
* **Port Çakışması:** Aynı makinede, aynı portu dinleyen iki uygulama olamaz (`EADDRINUSE` hatası alınır).

### 4. İşletim Sistemi Ağ Komutları
Aktif olarak dinlenen portları ve ağ durumunu kontrol etmek için kullandığım araçlar:
* **Ubuntu (Linux) Tarafı:** `ss -tuln` (Dinlenen portları hızlıca listeler) veya `lsof -i -P -n` (Portu hangi uygulamanın tuttuğunu gösterir).
* **Windows 10 Tarafı:** `netstat -ano` (Aktif portları ve süreç ID'lerini (PID) listeler).

---

## ✅ Tamamlananlar

* **Temel Soket Kurulumu:** `socket()` ile soket oluşturma ve `bind()` ile bir porta bağlama.

## 🚧 Öğrenilecekler / Sıradaki Adımlar (Genişletilmiş To-Do List)

1.  **Bağlantı Kabul Etme:**
    *   `listen()`: Soketi gelen bağlantıları dinleyecek şekilde pasif moda geçirme.
    *   `accept()`: Gelen bir bağlantı isteğini kabul ederek istemciyle iletişim için yeni bir soket (file descriptor) oluşturma.

2.  **Non-Blocking I/O ve I/O Multiplexing:**
    *   **Non-Blocking Soketler:** `fcntl()` kullanarak soketleri non-blocking moda ayarlama. Bu, `accept`, `recv`, `send` gibi fonksiyonların programı kilitlemesini önler.
    *   **Multiplexing:** `poll()` (veya `epoll`/`kqueue`) kullanarak aynı anda birden çok istemciyi yönetme.
        *   Ana dinleme soketini ve tüm istemci soketlerini izleme.
        *   Hangi soketin okumaya veya yazmaya hazır olduğunu verimli bir şekilde tespit etme.

3.  **HTTP Protokolü İşleme:**
    *   **Request Parsing:** İstemciden gelen HTTP isteklerini (GET, POST, DELETE) ayrıştırma.
        *   İstek satırını (örn: `GET /index.html HTTP/1.1`), başlıkları (headers) ve varsa gövdeyi (body) okuma.
    *   **Response Oluşturma:** Geçerli HTTP yanıtları oluşturma.
        *   Durum kodları (200 OK, 404 Not Found, vb.).
        *   Gerekli başlıklar (`Content-Type`, `Content-Length`, vb.).
        *   İstenen kaynağı (dosya içeriği) veya hata sayfasını yanıta ekleme.

4.  **Konfigürasyon Dosyası (`.conf`):**
    *   **Parsing:** Projenin başlangıcında bir konfigürasyon dosyasını okuyup ayrıştırma.
    *   **Ayarlar:**
        *   Birden çok sunucu (port) dinleme.
        *   Varsayılan hata sayfaları.
        *   İstemci gövde boyutu limiti.
        *   Route (yol) bazlı kurallar:
            *   İzin verilen HTTP metodları.
            *   HTTP yönlendirmeleri (redirection).
            *   Statik dosyaların sunulacağı kök dizin.
            *   Dizin listeleme (directory listing) açma/kapama.

5.  **Statik Site Sunumu:**
    *   İstek URI'sine göre istenen dosyayı bulma ve içeriğini HTTP yanıtı olarak gönderme.

6.  **CGI (Common Gateway Interface):**
    *   Belirli isteklere (örn: `.php` uzantılı dosyalara) dayalı olarak harici bir program (CGI script) çalıştırma.
    *   İstek bilgilerini environment variables aracılığıyla CGI script'ine geçirme.
    *   CGI script'inin çıktısını yakalayıp istemciye HTTP yanıtı olarak gönderme.

7.  **Diğer Gereksinimler:**
    *   `Makefile` oluşturma (`all`, `clean`, `fclean`, `re` hedefleriyle).
    *   `C++98` standardına ve `-Wall -Wextra -Werror` flag'lerine uyum.
    *   Stres testi yaparak sunucunun çökmediğinden emin olma.
    *   Dosya yükleme (file upload) işlevselliği (POST metodu ile).