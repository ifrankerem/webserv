#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include "../includes/HttpRequest.hpp"
#include "../includes/HttpResponse.hpp"

enum ClientState {
    READING,
    PROCESSING, 
    WRITING,
    CLOSING
};

class Client {
private:
    int                 _fd;
    struct sockaddr_in  _address;
    ClientState         _state;
    
    HttpRequest         _request;  
    HttpResponse        _response;

public:
    Client();
    Client(int fd, struct sockaddr_in &address);
    ~Client();

    int getFd() const;
    ClientState getState() const;
    void setState(ClientState state);

    // HttpRequest& getRequestObj();
    // HttpResponse& getResponseObj();
};

#endif