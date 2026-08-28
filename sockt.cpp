#include "sockt.hpp"

sockt::sockt()
{
	this->socket_nbr = -1; //for default there is no socket rn
}

sockt::sockt(int domain,int type , int protocol)
{
	this->socket_nbr = socket(domain,type,protocol);
}

sockt::sockt(const sockt& other)
{
}

sockt::~sockt()
{
	this->ft_close();
}

sockt& sockt::operator=(const sockt& other)
{
}

void sockt::ft_close()
{
	if(this->socket_nbr >= 0)
	{
		close(this->socket_nbr);
		this->socket_nbr = -1;
	}	
}

void sockt::init_addr(int family, uint16_t port, uint32_t addr)
{
	this->server_addr.sin_addr.s_addr = htonl(addr);
	this->server_addr.sin_port = htons(port);
	this->server_addr.sin_family = family;
	//!burada bir okuma farki var buraya geri donucem en temiz sekilde yazacak olursak mainde temz bir ip vericez burada donusum yapicaz bu sefer hata bakmasi daha kolay olucak
}

ssize_t sockt::ft_recv(int fd)
{
	char buf[4096];
	ssize_t nbr = recv(fd,buf,sizeof(buf),0);
	if(nbr > 0)
	{
		this->message.append(buf,nbr);
		return nbr;
	}
	else //TODO else if for negative and adding exception
		return nbr;
}

std::string sockt::getMessage()
{
	return(this->message);
}

void sockt::ft_send(int fd,std::string request) 
{
	int flag = request.size();
	while(flag > 0)
	{
		ssize_t sent = send(fd, request.c_str(), flag, 0);
		// if sent == -1 exceptation !!
		flag -= sent;
	}
}

int sockt::getSocket_nbr()
{
	return(this->socket_nbr);
}

void sockt::clearMessage()
{
	this->message.clear();
}
