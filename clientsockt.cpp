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


int clientsockt::ft_connect()
{
	return(connect(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr)));
	//!castin eksik konu cpp06
}
