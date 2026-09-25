#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

// Müşterinin o anki durumu (State Machine)
enum ClientState {
    READING,    // İsteği (Request) okuma aşaması
    PROCESSING, // Okuma bitti, isteği ayrıştırma ve cevabı (Response) hazırlama aşaması
    WRITING,    // Hazırlanan cevabı müşteriye gönderme aşaması
    CLOSING     // İşlem bitti, bağlantıyı kapatma aşaması
};

class Client {
private:
    int                 _fd;
    struct sockaddr_in  _address;
    std::string         _request_buffer;
    std::string         _response_buffer;
    ClientState         _state;

public:
    Client();
    Client(int fd, struct sockaddr_in address);
    ~Client();

    // Getter & Setter
    int getFd() const;
    ClientState getState() const;
    void setState(ClientState state);

    // Buffer Yönetimi
    void appendRequest(const std::string& data);
    const std::string& getRequest() const;
    
    void setResponse(const std::string& res);
    const std::string& getResponse() const;
    void eraseResponse(size_t n); // Gönderilen kısmı buffer'dan silmek için
};

#endif