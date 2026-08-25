#include "sockt.hpp"

sockt::sockt()
{
	
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
}

sockt& sockt::operator=(const sockt& other)
{
}



