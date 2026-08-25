#include "clientsockt.hpp"

int main()
{
	//creating socket
	clientsockt *new_socket;
	new_socket = new clientsockt(AF_INET,SOCK_STREAM,0);
	//where i can connect with that socket
	new_socket->init_server_addr(AF_INET,9002,INADDR_ANY);

	new_socket->ft_connect();


	return 0;
}