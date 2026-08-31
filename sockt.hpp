#ifndef sockt_HPP
#define sockt_HPP


#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdexcept>
#include <cstring>   // strerror
#include <cerrno> 

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
		ssize_t ft_recv();
		std::string getMessage();
		int ft_send(int fd, const std::string &request);
		int getSocket_nbr();
		void clearMessage();

};

#endif
