#include "../includes/ServerManager.hpp"
#include "../includes/RequestHandler.hpp"

ServerManager::ServerManager(const Config& config){
    _config = config;
}
    
ServerManager::~ServerManager(){

}

const ServerConfig* findServerConfig(const Config& config, int port, const std::string& host_header) {
    const ServerConfig* default_server = NULL;
    for (size_t i = 0; i < config.servers.size(); i++) {
        for (size_t j = 0; j < config.servers[i].listens.size(); j++) {
            if (config.servers[i].listens[j].port == port) {
                if (default_server == NULL) default_server = &config.servers[i];
                if (!config.servers[i].server_name.empty() && host_header.find(config.servers[i].server_name) != std::string::npos) {
                    return &config.servers[i];
                }
            }
        }
    }
    return default_server;
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
            address.sin_addr.s_addr = inet_addr(_config.servers[i].listens[j].ip.c_str()); //kendi inetaddr ni yaz veya tip tanımla
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
    struct sockaddr_in cl;
    socklen_t cl_len = sizeof(cl);
    int new_client_fd = accept(server_fd, (struct sockaddr *)&cl, &cl_len);
    if( new_client_fd == -1)
    {
        std::cout << "error1";
    }
    fcntl(new_client_fd, F_SETFL, O_NONBLOCK);          
    
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = EPOLLIN; 
    event.data.fd = new_client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event); 
    
    Client new_client(new_client_fd,cl);
    new_client.setServerPort(_server_fds[server_fd]);
    _clients[new_client_fd] = new_client;
    _clients[new_client_fd].setState(READING);
}



void ServerManager::handleClientRead(int client_fd)
{
    char buffer[4096];
    int byte = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (byte > 0)
    {
        _clients[client_fd].feedData(buffer, byte);
        _clients[client_fd].parseData();

        if (_clients[client_fd].getParseState() == PARSE_DONE)
        {
            _clients[client_fd].getParserObj().printDebug();
            _clients[client_fd].setState(WRITING);

            struct epoll_event event;
            std::memset(&event, 0, sizeof(event));
            event.events = EPOLLOUT;
            event.data.fd = client_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        }
        else if (_clients[client_fd].getParseState() == PARSE_ERROR)
        {
            closeConnection(client_fd);
        }
    }
    else if (byte == 0)
    {
        closeConnection(client_fd);
    }
    else
    {
        closeConnection(client_fd);
    }
}

void ServerManager::handleClientWrite(int client_fd)
{
    Client& client = _clients[client_fd];

    if (!client.hasPendingData())
    {
        HttpRequest req = client.getParserObj().getParsedRequest();
        req.server_port = client.getServerPort();

        std::string host = "";
        if (req.headers.count("Host"))
            host = req.headers["Host"];

        const ServerConfig* srv = findServerConfig(_config, client.getServerPort(), host);
        if (srv == NULL) {
            closeConnection(client_fd);
            return;
        }

        RequestHandler handler(*srv);
        Step result = handler.handle(req);

        if (result.kind == Step::RESPOND) {
            client.prepareResponse(result.response.serialize());
        } else if (result.kind == Step::WAIT) {
            // İleride CGI için poll/epoll fd ekleme işlemleri buraya gelecek
            return;
        }
    }

    int bytes_sent = send(client_fd, client.sendPtr(), client.sendSize(), 0);

    if (bytes_sent > 0)
    {
        client.advanceSent(bytes_sent);
        if (!client.hasPendingData())
        {
            // Tüm veri gönderildi
            bool keep_alive = true;
            HttpRequest req = client.getParserObj().getParsedRequest();
            if (req.headers.count("Connection") && req.headers["Connection"] == "close")
                keep_alive = false;

            if (keep_alive) {
                client.resetForNextRequest();
                struct epoll_event event;
                std::memset(&event, 0, sizeof(event));
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
            } else {
                closeConnection(client_fd);
            }
        }
    }
    else
    {
        // 42 Subject kuralı: errno kontrolü yasak. -1 gelirse direkt kapatıyoruz.
        closeConnection(client_fd);
    }
}

void ServerManager::closeConnection(int client_fd)
{
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _clients.erase(client_fd);
}
