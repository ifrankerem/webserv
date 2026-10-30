#include "Client.hpp"
#include "RequestHandler.hpp"

Client::Client(): _fd(-1), _state(READING), _bytesSent(0), _serverPort(0), _handler(NULL), _lastActivity(std::time(NULL)) {

}

Client::Client(int fd, sockaddr_in &address): _fd(fd), _address(address), _state(READING), _bytesSent(0), _serverPort(0), _handler(NULL), _lastActivity(std::time(NULL))
{
}

// Veri her aktiginda cagrilir: zaman asimi sayaci bastan baslar.
// std::time C++98 standart kutuphanesinden (<ctime>); subject'in sistem cagrisi
// listesinde zaman fonksiyonu olmadigi icin onu kullaniyoruz.
void Client::touch() {
    _lastActivity = std::time(NULL);
}

time_t Client::getLastActivity() const {
    return _lastActivity;
}

int Client::getFd() const{
    return _fd;
}

ClientState Client::getState() const{
    return _state;
}

void Client::setState(ClientState state){ 
    _state = state;
}

void Client::feedData(const char* chunk, int size)
{
    _parser.feed(chunk, size); 
}

void Client::parseData() {
    _parser.parse();
}

ParseState Client::getParseState() const {
    return _parser.getState();
}

HttpParser& Client::getParserObj() {
    return _parser;
}

void Client::resetForNextRequest() {
    _parser.reset();
    if (_writeBuffer.capacity() > 64 * 1024) {
        std::string().swap(_writeBuffer);
    } else {
        _writeBuffer.clear();
    }
    _bytesSent = 0;
    _state = READING;
    clearHandler();
}

Client::~Client()
{
    clearHandler();
}

void Client::setServerPort(int port) { _serverPort = port; }
int Client::getServerPort() const { return _serverPort; }
void Client::setServerIp(const std::string& ip) { _serverIp = ip; }
const std::string& Client::getServerIp() const { return _serverIp; }

void Client::prepareResponse(const std::string& serialized) {
    _writeBuffer = serialized;
    _bytesSent = 0;
}
struct sockaddr_in Client::getAdress(){
    return _address;
};

bool Client::hasPendingData() const { 
    return !_writeBuffer.empty() && _bytesSent < _writeBuffer.length(); 
}

const char* Client::sendPtr() const { 
    return _writeBuffer.c_str() + _bytesSent; 
}

size_t Client::sendSize() const { 
    return _writeBuffer.length() - _bytesSent; 
}

void Client::advanceSent(size_t n) { 
    _bytesSent += n; 
}

RequestHandler* Client::getHandler() const {
    return _handler;
}

void Client::setHandler(RequestHandler* handler) {
    clearHandler();
    _handler = handler;
}

void Client::clearHandler() {
    if (_handler != NULL) {
        delete _handler;
        _handler = NULL;
    }
}