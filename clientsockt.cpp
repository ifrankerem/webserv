#include "clientsockt.hpp"

clientsockt::clientsockt() 
{ 
	this->sent_bytes = 0;
}

clientsockt::clientsockt(int fd) 
{
	this->socket_nbr = fd;
	this->sent_bytes = 0;
}



clientsockt::clientsockt(int domain,int type , int protocol) : sockt(domain,type,protocol)
{ 
	this->sent_bytes = 0;
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
	int connection_status = connect(this->socket_nbr,reinterpret_cast<const sockaddr*>(&this->server_addr),sizeof(this->server_addr));
	if(connection_status == -1)
		throw std::runtime_error(std::string("connect: " ) + strerror(errno));
	else
		return connection_status;
	//!castin eksik konu cpp06
}

bool clientsockt::ft_handleWrite()
{
	size_t remaining = this->writeBuffer.size() - this->sent_bytes;
	if(remaining == 0)
		return true;
	ssize_t n = send(this->socket_nbr,this->writeBuffer.c_str() + this->sent_bytes,remaining,0);
	if(n <= 0)
		return false;
	this->sent_bytes += n;
	return (sent_bytes == this->writeBuffer.size());
}

void clientsockt::setWriteBuffer(std::string data)
{
	this->writeBuffer = data;
}
