#include "clientsockt.hpp"
#include "parsing.hpp"
#include <csignal>

int main()
{
	signal(SIGPIPE, SIG_IGN); //TODO simdilik ekliyorum daha detayli arastirmasini yapicam
	//creating socket
	clientsockt *new_socket;
	new_socket = new clientsockt(AF_INET,SOCK_STREAM,0);
	//where i can connect with that socket
	new_socket->init_addr(AF_INET,8080,0x7F000001); //127.0.0.1 for testing

	int connection_status = new_socket->ft_connect();
	if(connection_status == -1) //TODO making exceptation
		std::cout << "There was an error while making connection to the remote socket" << std::endl;

	std::string request = "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n"; //headerlarim bitti demelisin yoksa sunucu surekli bkeler
	new_socket->ft_send(new_socket->getSocket_nbr(),request);


	long content_length = -1;
	size_t header_len = 0;
	while (1)
	{
		if (new_socket->ft_recv(new_socket->getSocket_nbr()) <= 0)
			break;

		if (content_length < 0)
		{
			std::string::size_type pos = new_socket->getMessage().find("\r\n\r\n");
			if (pos != std::string::npos)
			{
				header_len = pos + 4; // \r\n\r\n
				content_length = parsing::getContentLength(new_socket->getMessage());
			}
		}

		if (content_length >= 0 && new_socket->getMessage().size() >= header_len + content_length)
			break;
	}

	std::cout << new_socket->getMessage() << std::endl;


	delete(new_socket); //will call ft_close
	return 0;
	
}