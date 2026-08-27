#include "listensockt.hpp"

listensockt::listensockt()
{ 
}

listensockt::listensockt(int domain,int type , int protocol) : sockt(domain,type,protocol)
{ 
}

listensockt::listensockt(const listensockt& other)
{
}

listensockt::~listensockt()
{
}

listensockt& listensockt::operator=(const listensockt& other)
{
}

int listensockt::ft_bind()
{	
	//TODO these two function throws exceptation so handle it
	int value = 1;
	setsockopt(this->socket_nbr,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //TODO because of TIME_WAIT i have to use that kinda function i will look at it after for more detailed
	return(bind(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr)));
}

int listensockt::ft_listen()
{
	//TODO exceptation
	return (listen(this->socket_nbr,10)); // connect queue 
}


int listensockt::ft_accept()
{
	//TODO exceptation
	return(accept(this->socket_nbr,NULL,NULL));
}


