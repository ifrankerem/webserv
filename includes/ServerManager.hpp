#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include <map>
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <cstring>
#include "ConfigParser.hpp"
#include "Client.hpp"
#include "HttpResponse.hpp"

#define MAX_EVENTS 1024
// Bir client (ya da onun CGI'i) bu kadar saniye hic aktivite gostermezse kapatilir.
// Subject: "A request to your server should never hang indefinitely."
// Aktivite = recv/send ile veri akmasi ya da CGI pipe'inda olay olmasi.
#define IDLE_TIMEOUT 30

class ServerManager {
private:
    Config                          _config;
    int                             _epoll_fd;
    struct epoll_event              _events[MAX_EVENTS];
    
    std::map<int, int>              _server_fds; 
    
    std::map<int, Client>           _clients;
    std::map<int, int>              _cgi_to_client; // cgi_pipe_fd -> client_fd
    time_t                          _lastTimeoutScan; // checkTimeouts() son calistigi saniye

    void acceptConnection(int server_fd); 
    void handleClientRead(int client_fd);
    void handleClientWrite(int client_fd);
    void closeConnection(int client_fd);
    void handleCgiEvent(int cgi_fd, uint32_t events);
    void dropCgiFds(int client_fd);
    void checkTimeouts();
    void queueResponse(int client_fd, const HttpResponse& response);
    static void  signal_handler(int signal);

public:
    ServerManager(const Config& config);
    ~ServerManager();

    void setupServers(); 
    
    void run(); 
};

#endif