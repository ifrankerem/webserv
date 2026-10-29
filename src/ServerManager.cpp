#include "../includes/ServerManager.hpp"
#include "../includes/RequestHandler.hpp"
#include "../includes/Utils.hpp"
#include <csignal>
#include <poll.h>
#include <cerrno>

ServerManager::ServerManager(const Config& config){
    _config = config;
}
    
ServerManager::~ServerManager(){
    std::map<int, Client>::iterator it = _clients.begin();
    while (it != _clients.end()) {
        int fd = it->first;
        ++it;
        closeConnection(fd);
    }
    for (std::map<int, Listen>::iterator it = _server_fds.begin(); it != _server_fds.end(); ++it) {
        close(it->first);
    }
    _server_fds.clear();
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
}

int g_running = 1;

void ServerManager::signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

const ServerConfig* findServerConfig(const Config& config, const std::string& ip, int port, const std::string& host_header) {
    const ServerConfig* default_server = NULL;
    for (size_t i = 0; i < config.servers.size(); i++) {
        for (size_t j = 0; j < config.servers[i].listens.size(); j++) {
            if (config.servers[i].listens[j].port == port &&
                (config.servers[i].listens[j].ip == ip || config.servers[i].listens[j].ip == "0.0.0.0" || ip == "0.0.0.0")) {
                if (default_server == NULL) default_server = &config.servers[i];
                if (!config.servers[i].server_name.empty()) {
                    bool match = false;
                    for (size_t k = 0; k < config.servers[i].server_name.size(); k++) {
                        if (host_header.find(config.servers[i].server_name[k]) != std::string::npos) {
                            match = true;
                            break;
                        }
                    }
                    if (match) return &config.servers[i];
                }
            }
        }
    }
    return default_server;
}

bool listen_check(const std::vector<Listen> &listens, const std::string &ip, int port)
{
    for (size_t i = 0; i < listens.size(); i++)
    {
        if (listens[i].ip == ip && listens[i].port == port)
            return true;
    }
    return false;
}

void ServerManager::setupServers()
{
    // CGI borusu koptugunda write() SIGPIPE ile sunucuyu öldürmesin
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    _epoll_fd = epoll_create(1);

    std::vector<Listen> opened_listens;

    for (size_t i = 0; i < _config.servers.size(); i++)
    {
        for (size_t j = 0; j < _config.servers[i].listens.size(); j++)
        {
            const std::string& ip = _config.servers[i].listens[j].ip;
            int port = _config.servers[i].listens[j].port;

            if (listen_check(opened_listens, ip, port))
                continue;      
            
            int socket_fd = socket(AF_INET,SOCK_STREAM,0);
        
            int opt = 1;
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            fcntl(socket_fd, F_SETFL, O_NONBLOCK);

            struct sockaddr_in address;

            std::memset(&address,0,sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = utils::inet_addr(ip);
            address.sin_port = htons(port);
                    
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

            _server_fds[socket_fd] = _config.servers[i].listens[j];

            opened_listens.push_back(_config.servers[i].listens[j]);

            std::cout << ip << ":" << port << std::endl;
        }     
    }
    if (_server_fds.empty()) 
        throw std::runtime_error("No listening sockets could be bound.");
}

void ServerManager::run()
{
    while (true)
    {
        int count_fd = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000);
        if(count_fd == -1)
        {
            if (!g_running)
                break;
            continue;
        }
        for (int i = 0; i < count_fd; i++){
            int current_fd = _events[i].data.fd;
            if (_server_fds.count(current_fd) > 0 )
            {
                acceptConnection(current_fd);
            }
            else if (_cgi_to_client.count(current_fd) > 0)
            {
                handleCgiEvent(current_fd, _events[i].events);
            }
            else if (_clients.count(current_fd) == 0)
            {
                continue;
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
        checkTimeouts();
    }
}

void ServerManager::acceptConnection(int server_fd)
{
    struct sockaddr_in cl;
    socklen_t cl_len = sizeof(cl);
    int new_client_fd = accept(server_fd, (struct sockaddr *)&cl, &cl_len);
    if( new_client_fd == -1)
    {
        return;
    }
    fcntl(new_client_fd, F_SETFL, O_NONBLOCK);          
    
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = EPOLLIN; 
    event.data.fd = new_client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event); 
    
    Client new_client(new_client_fd,cl);
    new_client.setServerPort(_server_fds[server_fd].port);
    new_client.setServerIp(_server_fds[server_fd].ip);
    _clients[new_client_fd] = new_client;
    _clients[new_client_fd].setState(READING);
}

void ServerManager::handleClientRead(int client_fd)
{
    char buffer[4096];
    int byte = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (byte > 0)
    {
        _clients[client_fd].touch();
        _clients[client_fd].feedData(buffer, byte);
        _clients[client_fd].parseData();

        if (_clients[client_fd].getParseState() == PARSE_DONE)
        {
            
            _clients[client_fd].setState(WRITING);

            struct epoll_event event;
            std::memset(&event, 0, sizeof(event));
            event.events = EPOLLOUT | EPOLLRDHUP;
            event.data.fd = client_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        }
        else if (_clients[client_fd].getParseState() == PARSE_ERROR)
        {
            HttpResponse err = utils::defaultErrorPage(400);
            err.headers["Connection"] = "close";
            _clients[client_fd].prepareResponse(err.serialize());
            _clients[client_fd].setState(WRITING);

            struct epoll_event event;
            std::memset(&event, 0, sizeof(event));
            event.events = EPOLLOUT | EPOLLRDHUP;
            event.data.fd = client_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        }
    }
    else if (byte == -1)
    {
        // if (errno == EAGAIN || errno == EWOULDBLOCK)
        //     return;
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
        HttpRequest &req = client.getParserObj().getParsedRequest();
        req.server_port = client.getServerPort();

        if (req.version.compare(0, 5, "HTTP/") == 0 && req.version != "HTTP/1.0" && req.version != "HTTP/1.1")
        {
            HttpResponse err = utils::defaultErrorPage(505);
            err.headers["Connection"] = "close";
            client.prepareResponse(err.serialize());
        }
        else if ((req.method == "POST" || req.method == "PUT") &&
                 req.headers.count("content-length") == 0 &&
                 req.headers.count("transfer-encoding") == 0)
        {
            HttpResponse err = utils::defaultErrorPage(411);
            err.headers["Connection"] = "close";
            client.prepareResponse(err.serialize());
        }
        else
        {
            std::string host = "";
            if (req.headers.count("host"))
                host = req.headers["host"];

            if (client.getHandler() == NULL) {
                const ServerConfig* srv = findServerConfig(_config, client.getServerIp(), client.getServerPort(), host);
                if (srv == NULL) {
                    closeConnection(client_fd);
                    return;
                }
                client.setHandler(new RequestHandler(*srv));
            }

            Step result = client.getHandler()->handle(req);

            if (result.kind == Step::RESPOND) {
                client.prepareResponse(result.response.serialize(req.method == "HEAD"));
                client.clearHandler();
            } else if (result.kind == Step::WAIT) {
                for (size_t i = 0; i < result.watch.size(); i++) {
                    struct epoll_event event;
                    std::memset(&event, 0, sizeof(event));
                    if (result.watch[i].events & POLLIN)
                        event.events |= EPOLLIN;
                    if (result.watch[i].events & POLLOUT)
                        event.events |= EPOLLOUT;
                    
                    event.data.fd = result.watch[i].fd;
                    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, result.watch[i].fd, &event);
                    
                    _cgi_to_client[result.watch[i].fd] = client_fd;
                }
                
                client.setState(PROCESSING);
                client.touch();
                
                struct epoll_event c_event;
                std::memset(&c_event, 0, sizeof(c_event));
                c_event.events = EPOLLRDHUP;
                c_event.data.fd = client_fd;
                epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &c_event);
                return;
            }
        }
    }

    int bytes_sent = send(client_fd, client.sendPtr(), client.sendSize(), 0);

    if (bytes_sent > 0)
    {
        client.touch();
        client.advanceSent(bytes_sent);
        if (!client.hasPendingData())
        {
            // Tüm veri gönderildi
            bool keep_alive = true;
            HttpRequest &req = client.getParserObj().getParsedRequest();
            if (req.headers.count("connection") && utils::toLower(req.headers["connection"]) == "close")
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
    else if (bytes_sent == -1)
    {
        // if (errno == EAGAIN || errno == EWOULDBLOCK)
        //     return; 
        closeConnection(client_fd);
    }
    else
    {
        closeConnection(client_fd);
    }
}

void ServerManager::closeConnection(int client_fd)
{
    std::map<int, int>::iterator it = _cgi_to_client.begin();
    while (it != _cgi_to_client.end()) {
        if (it->second == client_fd) {
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
            _cgi_to_client.erase(it++);
        } else {
            ++it;
        }
    }

    if (_clients.find(client_fd) != _clients.end()) {
        _clients[client_fd].clearHandler();
    }

    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _clients.erase(client_fd);
}

void ServerManager::handleCgiEvent(int cgi_fd, uint32_t events)
{
    std::map<int, int>::iterator mapped = _cgi_to_client.find(cgi_fd);
    if (mapped == _cgi_to_client.end()) // bayat olay: bu fd artik bizim degil
        return;

    int client_fd = mapped->second;
    if (_clients.find(client_fd) == _clients.end()) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        _cgi_to_client.erase(cgi_fd);
        return;
    }

    Client& client = _clients[client_fd];
    if (client.getHandler() == NULL) return;
    client.touch();

    short poll_events = 0;
    if (events & EPOLLIN) poll_events |= POLLIN;
    if (events & EPOLLOUT) poll_events |= POLLOUT;
    if (events & EPOLLERR) poll_events |= POLLERR;
    if (events & EPOLLHUP) poll_events |= POLLHUP;

    Step result = client.getHandler()->onReady(cgi_fd, poll_events);

    for (size_t i = 0; i < result.unwatch.size(); i++) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, result.unwatch[i], NULL);
        close(result.unwatch[i]);
        _cgi_to_client.erase(result.unwatch[i]);
    }

    if (result.kind == Step::RESPOND) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        _cgi_to_client.erase(cgi_fd);

        client.prepareResponse(result.response.serialize(
            client.getParserObj().getParsedRequest().method == "HEAD"));
        client.clearHandler();

        client.setState(WRITING);
        struct epoll_event event;
        std::memset(&event, 0, sizeof(event));
        event.events = EPOLLOUT;
        event.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
    }
}

void ServerManager::checkTimeouts()
{
    time_t now = std::time(NULL);
    std::map<int, Client>::iterator it = _clients.begin();
    while (it != _clients.end())
    {
        int client_fd = it->first;
        Client& client = it->second;
        ++it;

        if (now - client.getLastActivity() > 30)
        {
            if (client.getState() == PROCESSING)
            {
                std::map<int, int>::iterator cgi_it = _cgi_to_client.begin();
                while (cgi_it != _cgi_to_client.end()) {
                    if (cgi_it->second == client_fd) {
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_it->first, NULL);
                        _cgi_to_client.erase(cgi_it++);
                    } else {
                        ++cgi_it;
                    }
                }

                Client& client_ref = _clients[client_fd];
                client_ref.prepareResponse(utils::defaultErrorPage(504).serialize());
                client_ref.clearHandler(); // ~RequestHandler() calisir: kill(pid, SIGKILL) ve waitpid yapar!
                client_ref.setState(WRITING);
                client_ref.touch();

                struct epoll_event event;
                std::memset(&event, 0, sizeof(event));
                event.events = EPOLLOUT;
                event.data.fd = client_fd;
                epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
            }
            else
            {
                closeConnection(client_fd);
            }
        }
    }
}
