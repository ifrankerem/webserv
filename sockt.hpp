#ifndef sockt_HPP
#define sockt_HPP


#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

//Abstract CLASS

class sockt
{
	protected:
		int socket_nbr;
		sockaddr_in server_addr;
		std::string message;

	public:
		sockt();
		sockt(int domain,int type , int protocol);
		virtual ~sockt() = 0;
		sockt(const sockt& other);
		sockt& operator=(const sockt& other);
		void ft_close();
		void init_addr(int family, uint16_t port, uint32_t addr);
		ssize_t ft_recv(int fd);
		std::string getMessage();
		void ft_send(int fd, std::string request);
		int getSocket_nbr();
		void clearMessage();

};

#endif
