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
	int value = 1;
	int flag = setsockopt(this->socket_nbr,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //TODO because of TIME_WAIT i have to use that kinda function i will look at it after for more detailed
	if(flag == -1)
		throw std::runtime_error(std::string("setsockopt: " ) + strerror(errno));
	int flag2 = bind(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr));
	if(flag2 == -1)
		throw std::runtime_error(std::string("bind: " ) + strerror(errno));
	else
		return flag2;
}

int listensockt::ft_listen()
{
	int flag = listen(this->socket_nbr,10); // connect queue 
	if(flag == -1)
		throw std::runtime_error(std::string("listen: " ) + strerror(errno));
	else 
		return flag;
}


int listensockt::ft_accept()
{
	int flag = accept(this->socket_nbr,NULL,NULL);
	return flag;
}


