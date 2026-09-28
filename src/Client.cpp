#include "../includes/Client.hpp"

Client::Client(){

}

Client::Client(int fd, sockaddr_in &address): _fd(fd), _address(address), _state(READING)
{
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
    _state = READING;
}

Client::~Client()
{

}