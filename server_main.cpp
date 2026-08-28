#include "listensockt.hpp"
#include "parsing.hpp"
#include <csignal>
#include <fstream>
#include <sstream>


int main()
{
		listensockt *new_socket;
	try{
		signal(SIGPIPE, SIG_IGN); //TODO simdilik ekliyorum daha detayli arastirmasini yapicam
		//creating socket
		new_socket = new listensockt(AF_INET,SOCK_STREAM,0);
		//where i can connect with that socket
		new_socket->init_addr(AF_INET,8080,0x7F000001); //127.0.0.1 for testing

		new_socket->ft_bind();

		new_socket->ft_listen();


		while(1)
		{

			int conn_fd = new_socket->ft_accept();
			if(conn_fd == -1)
				continue; //this connection cannot made it so continue
			new_socket->clearMessage();
			bool flag = true;
			while(1)
			{
				ssize_t x = new_socket->ft_recv(conn_fd);
				if(x <= 0) //error  case
				{
					flag = false;
					break;
				}
				if(new_socket->getMessage().find("\r\n\r\n") != std::string::npos)
				{
					break; //header comes successfuly!
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
			if(flag)
				new_socket->ft_send(conn_fd,content);
			close(conn_fd);
			//TODO connection closed and main closed properly
		}

		delete(new_socket); //will call ft_close
		return 0;
	}
	catch(std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		delete(new_socket);
		return 1;
	}
}