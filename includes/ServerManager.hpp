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

#define MAX_EVENTS 1024

class ServerManager {
private:
    Config                          _config;
    int                             _epoll_fd;
    struct epoll_event              _events[MAX_EVENTS];
    
    std::map<int, int>              _server_fds; 
    
    std::map<int, Client>           _clients;

    void acceptConnection(int server_fd); 
    void handleClientRead(int client_fd);
    void handleClientWrite(int client_fd);
    void closeConnection(int client_fd);

public:
    ServerManager(const Config& config);
    ~ServerManager();

    void setupServers(); 
    
    void run(); 
};

#endif