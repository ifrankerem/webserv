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
