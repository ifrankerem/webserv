#include "../includes/ServerManager.hpp"

ServerManager::ServerManager(const Config& config){
    _config = config;
}
    
ServerManager::~ServerManager(){

}

bool port_check(std::vector<int> &ports, int current)
{
    for (size_t i = 0; i < ports.size(); i++)
    {
        if (ports[i] == current)
            return true;
    }
    return false;
}

void ServerManager::setupServers()
{
    _epoll_fd = epoll_create1(0);

    std::vector<int> ports;

    for (size_t i = 0; i < _config.servers.size(); i++)
    {
        for (size_t j = 0; j < _config.servers[i].listens.size(); j++)
        {
      
            if (port_check(ports,_config.servers[i].listens[j].port))
                continue;      
            
            int socket_fd = socket(AF_INET,SOCK_STREAM,0);
        
            int opt = 1;
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            fcntl(socket_fd, F_SETFL, O_NONBLOCK);

            struct sockaddr_in address;

            std::memset(&address,0,sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = inet_addr(_config.servers[i].listens[j].ip.c_str());
            address.sin_port = htons(_config.servers[i].listens[j].port);
                    
            if(bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) == -1)
            {
                std::cerr << "Hata: Bind core failed)" << std::endl;
                close(socket_fd);
                continue;
            }

            if (listen(socket_fd,SOMAXCONN) == -1)
            {
                std::cerr << "Hata: Listen failed!" << std::endl;
                close(socket_fd);
                continue;
            }

            struct epoll_event event;
            std::memset(&event, 0, sizeof(event));
            event.events = EPOLLIN;
            event.data.fd = socket_fd;
            
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) 
            {
                std::cerr << "Hata: epoll_ctl failed!" << std::endl;
                close(socket_fd);
                continue;
            }

            _server_fds[socket_fd] = _config.servers[i].listens[j].port;

            ports.push_back(_config.servers[i].listens[j].port);

            std::cout << "Port " << _config.servers[i].listens[j].port << std::endl;
        }     
    }
}

void ServerManager::run()
{
    while (true)
    {
        int count_fd = epoll_wait(_epoll_fd, _events, MAX_EVENTS, -1);
        if(count_fd == -1)
        {
            std::cout << "Error: Ctrl + c";
            break;
        }
        for (int i = 0; i < count_fd; i++){
            int current_fd = _events[i].data.fd;

            if (_server_fds.count(current_fd) > 0 )
            {
                acceptConnection(current_fd);
            }
            else if (_events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                closeConnection(current_fd);
            }
            else if ((_events[i].events & EPOLLIN) && _clients[current_fd].getState() == READING)
            {
                handleClientRead(current_fd);
            }
            else if ((_events[i].events & EPOLLOUT) && _clients[current_fd].getState() == WRITING)
            {
                handleClientWrite(current_fd);
            }
            
        }
    }
}

void ServerManager::acceptConnection(int server_fd)
{

    (void)server_fd;
}

void ServerManager::handleClientRead(int client_fd)
{

}

void ServerManager::handleClientWrite(int client_fd)
{
    (void)client_fd;
}

void ServerManager::closeConnection(int client_fd)
{
    (void)client_fd;
}
