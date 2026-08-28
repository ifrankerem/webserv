#include "listensockt.hpp"
#include "parsing.hpp"
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
		new_socket->clearMessage();
		while(new_socket->ft_recv(conn_fd) != 0)
		{
			std::string::size_type pos = new_socket->getMessage().find("\r\n\r\n");
			if(pos == std::string::npos)
			{
				// continue
			}
			else
			{
				break;
			}
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
		std::string content_length_text = "content-length: ";
		std::string content_length_str = parsing::to_str(content.length());
		content_length_text.append(content_length_str);
		content_length_text.append("\r\n\r\n");
		content = content.insert(0,content_length_text);
		new_socket->ft_send(conn_fd,content);
		close(conn_fd);
		//TODO connection closed and main closed properly
	}

	delete(new_socket); //will call ft_close
	return 0;
	
}