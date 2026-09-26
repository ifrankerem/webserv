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


void Client::setState(ClientState state){ _state = state;}


// HttpRequest& Client::getRequestObj(){
    
//     return ;
// }
// HttpResponse& Client::getResponseObj(){
//     return ;
// }

Client::~Client()
{

}