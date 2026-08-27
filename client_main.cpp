#include "clientsockt.hpp"
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
	new_socket->ft_send(request);
	// while(new_socket->ft_recv(new_socket->getSocket_nbr()) != 0)
	// {
	// }
		
	std::cout << new_socket->getMessage() << std::endl;

	delete(new_socket); //will call ft_close
	return 0;
	
}