#ifndef CLIENTSOCKT_HPP
#define CLIENTSOCKT_HPP

#include "sockt.hpp"

class clientsockt : public sockt
{
	private:

	public:
		clientsockt();
		clientsockt(int domain, int type , int protocol);
		~clientsockt();
		clientsockt(const clientsockt& other);
		clientsockt& operator=(const clientsockt& other);
		int ft_connect();	
};

#endif