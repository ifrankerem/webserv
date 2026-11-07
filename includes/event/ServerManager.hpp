#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "../core/ConfigParser.hpp"
#include "Client.hpp"
#include "../http/HttpResponse.hpp"
#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>

#define MAX_EVENTS 1024

class ServerManager 
{
    private:
        Config                _config;
        int                   _epoll_fd;
        struct epoll_event    _events[MAX_EVENTS];
        std::map<int, Listen> _server_fds; 
        std::map<int, Client> _clients;
        std::map<int, int>    _cgi_to_client;

        void            acceptConnection(int server_fd); 
        void            handleClientRead(int client_fd);
        void            handleClientWrite(int client_fd);
        void            closeConnection(int client_fd);
        void            handleCgiEvent(int cgi_fd, uint32_t events);
        void            checkTimeouts();
        HttpResponse    errorResponse(Client& client, int status);
        static void     signal_handler(int signal);

    public:
        ServerManager(const Config& config);
        ~ServerManager();

        void setupServers(); 
        void run(); 
};

#endif