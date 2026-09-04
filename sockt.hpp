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
		std::string writeBuffer;
		std::string readBuffer;

	public:
		sockt();
		sockt(int domain,int type , int protocol);
		virtual ~sockt() = 0;
		sockt(const sockt& other);
		sockt& operator=(const sockt& other);
		void ft_close();
		void init_addr(int family, uint16_t port, uint32_t addr);
		ssize_t ft_recv();
		int ft_send(int fd, const std::string &data);
		int getSocket_nbr();
		std::string getWriteBuffer();
		std::string getReadBuffer();

};

#endif
