#include "sockt.hpp"

sockt::sockt()
{
	//maybe printin an error
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
	close(this->socket_nbr);
}

sockt& sockt::operator=(const sockt& other)
{
}



