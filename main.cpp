#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream> // ostringstream için gerekli
#include <string>
#include <fstream>

void execute()
{
 // 1. SOKET OLUŞTUR
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. ADRES HAZIRLA
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    // 3. BIND
    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // 4. LISTEN
    listen(server_socket, 10);
    std::cout << "Sunucu hazir! http://localhost:8080" << std::endl;

    // 5. SONSUZ DÖNGÜ (Sürekli açık kalması için)
    while (true){
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);

        // Müşteriyi (Tarayıcıyı) içeri al
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);

        if (client_socket != -1) {
            // --- 1. AŞAMA: Tarayıcının bize gönderdiği isteği (GET) okuyalım ---
            char buffer[2048] = {0}; 
            read(client_socket, buffer, 2048); // Öğrenmek için okuyoruz ve ekrana basıyoruz
            std::cout << "--- YENI ISTEK GELDI ---" << std::endl;
            std::cout << buffer << std::endl; // Terminalde Chrome'un sana ne yolladığını göreceksin!

            // --- 2. AŞAMA: Tarayıcıya HTML cevap gönderelim ---
            // html_body içeriğini index.html dosyasından oku
            std::ifstream html_file("html/index.html");
            std::string html_body;

            if (html_file) {
                std::ostringstream html_stream;
                html_stream << html_file.rdbuf();
                html_body = html_stream.str();
            } else {
                html_body = "<!DOCTYPE html><html><body><h1>index.html bulunamadi</h1></body></html>";
            }

            // Şimdi bunu HTTP kurallarına göre paketleyelim (Headers + Body)
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: " << html_body.length() << "\r\n";
            response << "\r\n"; // Başlıkların bittiğini gösteren sihirli boş satır
            response << html_body; // Ve HTML gövdemiz

            std::string final_response = response.str();

            // Hazırladığımız paketi tarayıcıya yolluyoruz
            write(client_socket, final_response.c_str(), final_response.length()); 
        
            // --- 3. AŞAMA: Müşteriyle işimiz bitti, soketi kapatıyoruz ---
            close(client_socket); 
        }
    }

    close(server_socket);
}


int main() {
    execute();
}