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

void clientsockt::ft_connect()
{
	connect(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr));
	//castin eksik konu cpp06
}
