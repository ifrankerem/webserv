#include "sockt.hpp"

sockt::sockt()
{
	this->socket_nbr = -1; //for default there is no socket rn
}

sockt::sockt(int domain,int type , int protocol)
{
	this->socket_nbr = socket(domain,type,protocol);
	if(this->socket_nbr == -1)
		throw std::runtime_error(std::string("socket: " ) + strerror(errno));
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

ssize_t sockt::ft_recv()
{
	char buf[4096];
	ssize_t nbr = recv(this->socket_nbr,buf,sizeof(buf),0);
	if(nbr > 0)
	{
		this->readBuffer.append(buf,nbr);
		return nbr;
	}
	else
		return nbr;
}


int sockt::ft_send(int fd, const std::string &data) 
{
	size_t size = data.size();
	size_t flag = 0;
	while(size > 0)
	{
		ssize_t sent = send(fd, data.c_str() + flag, size, 0);
		if(sent <= 0)
			return -1;
		flag += sent;
		size -= sent;
	}
	return flag;
}

int sockt::getSocket_nbr()
{
	return(this->socket_nbr);
}

std::string sockt::getWriteBuffer()
{
	return(this->writeBuffer);
}

std::string sockt::getReadBuffer()
{
	return(this->readBuffer);
}
