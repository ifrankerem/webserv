#ifndef CLIENTSOCKT_HPP
#define CLIENTSOCKT_HPP

#include "sockt.hpp"

class clientsockt : public sockt
{
	private:
		ssize_t sent_bytes;
		std::string writeBuffer;
		std::string readBuffer;
		HttpRequest request;
		HttpResponse response;

	public:
		clientsockt();
		clientsockt(int fd);
		clientsockt(int domain, int type , int protocol);
		~clientsockt(); 
		clientsockt(const clientsockt& other);
		clientsockt& operator=(const clientsockt& other);
		int ft_connect();
		void setWriteBuffer(std::string data);
		bool ft_handleWrite();


};

#endif
