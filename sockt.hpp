#ifndef sockt_HPP
#define sockt_HPP


#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


class sockt
{
	protected:
		int socket_nbr;

	public:
		sockt();
		sockt(int domain,int type , int protocol);
		~sockt();
		sockt(const sockt& other);
		sockt& operator=(const sockt& other);
};

#endif
