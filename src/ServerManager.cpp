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
    _clients[new_client_fd] = new_client;
    _clients[new_client_fd].setState(READING);
}

void HttpParser::printDebug() const { // bunu sil
    std::cout << "\n========== [ PARSER DEBUG ] ==========" << std::endl;
    std::cout << "Method  : [" << _request.method << "]" << std::endl;
    std::cout << "Path    : [" << _request.path << "]" << std::endl;
    std::cout << "Query   : [" << _request.query << "]" << std::endl;
    std::cout << "Version : [" << _request.version << "]" << std::endl;
    
    std::cout << "--- Headers (" << _request.headers.size() << ") ---" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin(); 
         it != _request.headers.end(); ++it) {
        std::cout << "  '" << it->first << "' : '" << it->second << "'" << std::endl;
    }
    
    std::cout << "--- Body (" << _request.body.length() << " bytes) ---" << std::endl;
    if (_request.body.empty())
        std::cout << "  (Body Bos)" << std::endl;
    else
        std::cout << _request.body << std::endl;
    std::cout << "======================================\n" << std::endl;
}

void ServerManager::handleClientRead(int client_fd)
{
    char buffer[4096];
    int byte;
    byte = recv(client_fd,buffer,sizeof(buffer)-1, 0);
    if (byte > 0)
    {
       _clients[client_fd].feedData(buffer,byte);
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
        else if (byte == 0)
        {
            closeConnection(client_fd);
        }       
    }
}

void ServerManager::handleClientWrite(int client_fd)
{
    std::ifstream file("html/index.html");
    
    char c;
    std::string test_site = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 1441\r\n"
        "\r\n";
    while (file.get(c))
    {
        test_site += c;
    }
    int bytes_sent = send(client_fd, test_site.c_str(), test_site.length(), 0);

    if (bytes_sent > 0)
    {
        std::cout << "Cevap gönderildi! (" << bytes_sent << " bayt)" << std::endl;
                closeConnection(client_fd); 
    }
    else
    {
        std::cerr << "Gönderme işlemi başarısız!" << std::endl;
        closeConnection(client_fd);
    }
}

void ServerManager::closeConnection(int client_fd)
{
    _server_fds.erase(client_fd);
    _clients.erase(client_fd);
    (void)client_fd;
}
