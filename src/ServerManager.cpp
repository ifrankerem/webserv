#include "../includes/ServerManager.hpp"
#include "../includes/RequestHandler.hpp"
#include "../includes/Utils.hpp"
#include <csignal>
#include <poll.h>
#include <cerrno>
#include <ctime>

ServerManager::ServerManager(const Config& config) : _lastTimeoutScan(0) {
    _config = config;
}
    
ServerManager::~ServerManager(){
    std::map<int, Client>::iterator it = _clients.begin();
    while (it != _clients.end()) {
        int fd = it->first;
        ++it;
        closeConnection(fd);
    }
    for (std::map<int, int>::iterator it = _server_fds.begin(); it != _server_fds.end(); ++it) {
        close(it->first);
    }
    _server_fds.clear();
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
}

// Sinyal handler'indan yazilan degisken: standart bunun icin volatile sig_atomic_t ister,
// boylece derleyici run()'daki okumayi optimize edip atamaz.
volatile sig_atomic_t g_running = 1;

void ServerManager::signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

const ServerConfig* findServerConfig(const Config& config, int port, const std::string& host_header) {
    const ServerConfig* default_server = NULL;
    for (size_t i = 0; i < config.servers.size(); i++) {
        for (size_t j = 0; j < config.servers[i].listens.size(); j++) {
            if (config.servers[i].listens[j].port == port) {
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
    // CGI borusu koptugunda write() SIGPIPE ile sunucuyu öldürmesin
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    _epoll_fd = epoll_create(1);

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

            std::cout << "127.0.0.1:" << _config.servers[i].listens[j].port << std::endl;
        }     
    }
}

void ServerManager::run()
{
    // Ctrl+C sunucu bir istegi islerken gelirse epoll_wait kesilmez; eskiden bayrak
    // sadece epoll_wait -1 donunce okundugu icin o Ctrl+C kayboluyordu. Dongu artik
    // en gec saniyede bir basa dondugu icin kosulda okumak yetiyor.
    while (g_running)
    {
        // -1 verseydik epoll_wait olay gelmedikce hic donmezdi; tek bir asili client
        // varken timeout kontrolu de hic calismazdi. 1000 ms: olay olmasa bile
        // saniyede bir uyanip checkTimeouts()'a ugruyoruz.
        int count_fd = epoll_wait(_epoll_fd, _events, MAX_EVENTS, 1000);
        if(count_fd == -1)
            continue; // EINTR (ornegin SIGINT): dongu kosulu g_running'e bakar
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
            else if (_clients.find(current_fd) == _clients.end())
            {
                // Bayat olay: bu fd ayni epoll turunda daha once kapandi
                // (ornegin CGI stdin'i, stdout'u bitince kapatildi).
                // Eskiden asagidaki _clients[current_fd] burada sahte bir Client
                // kaydi aciyordu. checkTimeouts() 30 sn sonra o sahte kaydi kapatirken
                // fd numarasi o arada baska birine verildiyse onu kapatirdi.
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
        // Olaylar islendikten sonra: sessiz kalanlari temizle
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
        _clients[client_fd].touch();
        _clients[client_fd].feedData(buffer, byte);
        _clients[client_fd].parseData();

        if (_clients[client_fd].getParseState() == PARSE_DONE)
        {
            
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

        std::string host = "";
        if (req.headers.count("host"))
            host = req.headers["host"];

        if (client.getHandler() == NULL) {
            const ServerConfig* srv = findServerConfig(_config, client.getServerPort(), host);
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
            
            struct epoll_event c_event;
            std::memset(&c_event, 0, sizeof(c_event));
            c_event.events = 0; // İstemciye dönene kadar bekle
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &c_event);
            return;
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
    if (_clients.find(client_fd) != _clients.end()) {
        _clients[client_fd].clearHandler();
    }

    dropCgiFds(client_fd);

    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _clients.erase(client_fd);
}

// Client'a ait CGI pipe'larini epoll'den ve _cgi_to_client tablosundan cikarir.
// fd'leri KAPATMAZ: onlar RequestHandler'a ait, onun destructor'i kapatir.
// Hem closeConnection() hem checkTimeouts() kullanir.
void ServerManager::dropCgiFds(int client_fd)
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
}

// Her epoll turunun sonunda calisir. IDLE_TIMEOUT saniyedir sessiz olan client'lar:
//
//   READING    -> client hic/yarim istek gonderip durdu (bos nc, yarim header,
//                 Content-Length'ten az body). Baglanti kapatilir.
//   WRITING    -> client cevabimizi okumuyor (socket buffer'i dolu). Kapatilir.
//   PROCESSING -> CGI calisiyor ama 30 sn'dir ne okudu ne yazdi (sonsuz dongu,
//                 kilitlenme). CGI oldurulur, client'a 504 Gateway Timeout gider.
void ServerManager::checkTimeouts()
{
    time_t now = std::time(NULL);
    // Yuk altinda epoll saniyede binlerce kez doner; zaman cozunurlugu 1 saniye
    // oldugu icin ayni saniye icinde tum client'lari tekrar gezmek bosa is.
    if (now == _lastTimeoutScan)
        return;
    _lastTimeoutScan = now;

    std::map<int, Client>::iterator it = _clients.begin();
    while (it != _clients.end()) {
        int fd = it->first;
        Client& client = it->second;
        // closeConnection() bu kaydi map'ten siler; silinen elemanin iterator'u
        // gecersiz olur, o yuzden silmeden ONCE bir sonrakine geciyoruz.
        ++it;

        if (now - client.getLastActivity() < IDLE_TIMEOUT)
            continue;

        if (client.getState() != PROCESSING) {
            closeConnection(fd);
            continue;
        }

        // 1) CGI pipe'larini epoll'den cikar ki kapanan fd'ler icin olay gelmesin
        dropCgiFds(fd);

        // 2) 504 cevabini hazirla. error() conf'taki error_page 504'u kullanir.
        //    PROCESSING'e ancak findServerConfig bir server bulduysa girilebilir,
        //    ayni girdilerle tekrar cagrildigi icin burada NULL donmez.
        HttpRequest& req = client.getParserObj().getParsedRequest();
        const ServerConfig* srv = findServerConfig(_config, client.getServerPort(),
            utils::getHeader(req.headers, "host"));

        // 3) Gonder: queueResponse handler'i siler; destructor'i CGI'a SIGKILL atar,
        //    waitpid ile toplar (zombie kalmaz) ve pipe fd'lerini kapatir
        queueResponse(fd, RequestHandler::error(504, srv));
    }
}

// Hazir cevabi client'in yazma buffer'ina koyar ve client'i yazma moduna alir.
// Handler'in isi bittigi icin silinir (CGI varsa destructor'i onu toplar).
// CGI'in normal bitisi (handleCgiEvent) ve CGI timeout'u (checkTimeouts) kullanir.
void ServerManager::queueResponse(int client_fd, const HttpResponse& response)
{
    Client& client = _clients[client_fd];
    client.prepareResponse(response.serialize(
        client.getParserObj().getParsedRequest().method == "HEAD"));
    client.clearHandler();

    client.setState(WRITING);
    client.touch(); // cevabin gonderilmesi icin sure bastan baslar
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = EPOLLOUT;
    event.data.fd = client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
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
    // CGI'dan olay geldi = hala calisiyor. Sayac sifirlanir; boylece yavas ama
    // veri ureten CGI (ornegin 100 MB'lik tester testleri) oldurulmez,
    // sadece tamamen sessiz kalan CGI timeout'a duser.
    client.touch();

    short poll_events = 0;
    if (events & EPOLLIN) poll_events |= POLLIN;
    if (events & EPOLLOUT) poll_events |= POLLOUT;
    if (events & EPOLLERR) poll_events |= POLLERR;
    if (events & EPOLLHUP) poll_events |= POLLHUP;

    Step result = client.getHandler()->onReady(cgi_fd, poll_events);

    for (size_t i = 0; i < result.unwatch.size(); i++) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, result.unwatch[i], NULL);
        _cgi_to_client.erase(result.unwatch[i]);
    }

    if (result.kind == Step::RESPOND) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi_fd, NULL);
        _cgi_to_client.erase(cgi_fd);
        queueResponse(client_fd, result.response);
    }
}
