#include "clientsockt.hpp"

clientsockt::clientsockt() 
{ 
}


clientsockt::clientsockt(int domain,int type , int protocol) : sockt(domain,type,protocol)
{ 
}

clientsockt::clientsockt(const clientsockt& other)
{
}

clientsockt::~clientsockt()
{
}

clientsockt& clientsockt::operator=(const clientsockt& other)
{
}

void clientsockt::init_server_addr(int family, uint16_t port, uint32_t addr)
{
	this->server_addr.sin_addr.s_addr = htonl(addr);
	this->server_addr.sin_port = htons(port);
	this->server_addr.sin_family = family;
	//!burada bir okuma farki var buraya geri donucem en temiz sekilde yazacak olursak mainde temz bir ip vericez burada donusum yapicaz bu sefer hata bakmasi daha kolay olucak
}

int clientsockt::ft_connect()
{
	return(connect(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr)));
	//castin eksik konu cpp06
}

ssize_t clientsockt::ft_recv()
{
	char buf[4096];
	ssize_t nbr = recv(this->socket_nbr,buf,sizeof(buf),0);
	if(nbr > 0)
	{
		this->server_response.append(buf,nbr);
		return nbr;
	}
	else //TODO else if for negative and adding exception
		return nbr;
}

std::string clientsockt::getServer_response()
{
	return this->server_response;
}

void clientsockt::ft_send()
{
	std::string request = "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n"; //headerlarim bitti demelisin yoksa sunucu surekli bkeler
	int flag = request.size();
	while(flag != 0)
	{
		ssize_t sent = send(this->socket_nbr, request.c_str(), request.size(), 0);
		flag -= sent;
	}
}