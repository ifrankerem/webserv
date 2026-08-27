#include "listensockt.hpp"
#include <csignal>
#include <fstream>
#include <sstream>


int main()
{
	signal(SIGPIPE, SIG_IGN); //TODO simdilik ekliyorum daha detayli arastirmasini yapicam
	//creating socket
	listensockt *new_socket;
	new_socket = new listensockt(AF_INET,SOCK_STREAM,0);
	//where i can connect with that socket
	new_socket->init_addr(AF_INET,8080,0x7F000001); //127.0.0.1 for testing

	new_socket->ft_bind();

	new_socket->ft_listen();


	while(1)
	{

		int conn_fd = new_socket->ft_accept();
		while(new_socket->ft_recv(conn_fd) != 0)
		{
		}

		std::cout << new_socket->getMessage() << std::endl;

		std::stringstream ss;
		std::ifstream file("index.html", std::ios::binary);
		if (!file.is_open())
		{
			//TODO 404
		}
		else
			ss << file.rdbuf();
		std::string content = ss.str();
		new_socket->ft_send(content);
		close(conn_fd);
		//TODO connection closed and main closed properly
	}

	delete(new_socket); //will call ft_close
	return 0;
	
}