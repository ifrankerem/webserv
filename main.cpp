#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in yapısı için gerekli
#include <unistd.h>

int main() {
    // 1. SOKETİ OLUŞTUR (Telefonu aldık)
    int my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (my_socket == -1) {
        std::cout << "Soket oluşturulamadı!" << std::endl;
        return 1;
    }

    // 2. ADRES BİLGİLERİNİ HAZIRLA (SIM kartı ve numarayı ayarlıyoruz)
    struct sockaddr_in server_address;
    
    server_address.sin_family = AF_INET;         // IPv4 kullanacağımızı tekrar belirtiyoruz
    server_address.sin_addr.s_addr = INADDR_ANY; // Bilgisayardaki tüm IP'lerden gelen istekleri dinle (0.0.0.0)
    server_address.sin_port = htons(8080);       // Port 8080. htons() byte sıralamasını işletim sisteminden ağ formatına çevirir.

    // 3. SOKETİ ADRESE BAĞLA (Numarayı telefona tanımla)
    // bind fonksiyonu genel bir sockaddr yapısı beklediği için sockaddr_in'i cast ediyoruz
    if (bind(my_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cout << "Bind işlemi başarısız oldu! (Port dolu olabilir)" << std::endl;
        close(my_socket);
        return 1;
    }

    std::cout << "Soket başarıyla 8080 portuna bağlandı!" << std::endl;

    // ... ileride buraya dinleme (listen) ve kabul etme (accept) gelecek ...

    // 4. SOKETİ KAPAT
    close(my_socket);

    return 0;
}
