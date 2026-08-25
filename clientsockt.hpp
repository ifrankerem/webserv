#ifndef CLIENTSOCKT_HPP
#define CLIENTSOCKT_HPP

#include "sockt.hpp"

class clientsockt : public sockt
{
	private:
		sockaddr_in server_addr;

	public:
		clientsockt();
		clientsockt(int domain, int type , int protocol);
		~clientsockt();
		clientsockt(const clientsockt& other);
		clientsockt& operator=(const clientsockt& other);
		void init_server_addr(int family, uint16_t port, uint32_t addr);
		void ft_connect();
};

#endif
