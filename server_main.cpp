#include "listensockt.hpp"
#include "clientsockt.hpp"
#include "parsing.hpp"
#include <csignal>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <poll.h>



std::string ft_make_dummyheader()
{
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
	std::string dummy_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
	content_length_text.append(content_length_str);
	dummy_header.append(content_length_text);
	dummy_header.append("\r\n\r\n");
	dummy_header.append(content);
	return dummy_header;
}

int main()
{
	listensockt *listen_socket;
	clientsockt *client_socket;
	std::vector <struct pollfd> pollfds;
	std::vector <struct pollfd> pendingfds;
	std::map<int,clientsockt*> connections;
	std::vector <int> closing_fds;

	try{
		signal(SIGPIPE, SIG_IGN); //TODO simdilik ekliyorum daha detayli arastirmasini yapicam
		//creating socket
		listen_socket = new listensockt(AF_INET,SOCK_STREAM,0);
		//where i can connect with that socket
		listen_socket->init_addr(AF_INET,8080,0x7F000001); //127.0.0.1 for testing

		listen_socket->ft_bind();

		listen_socket->ft_listen();
		struct pollfd pfd = pollfd(); // no memset so recreate the struct
		pfd.fd = listen_socket->getSocket_nbr(); //adding listen sockt to q
		pfd.events = POLLIN;
		pollfds.push_back(pfd);

		while(1)
		{
			poll(&pollfds[0],pollfds.size(),-1); //waiting unlimited
			for(size_t i = 0; i < pollfds.size();i++)
			{
				if(pollfds[i].revents == 0)
					continue; //nothing happens in this socket
				if(pollfds[i].fd == listen_socket->getSocket_nbr()) // this is the listen_fd
				{
					int conn_fd = listen_socket->ft_accept();
					if(conn_fd == -1)
						continue; //this connection cannot made it so continue
					struct pollfd client_sckt = pollfd(); // no memset so recreate the struct
					client_sckt.fd = conn_fd;
					client_sckt.events = POLLIN;
					pendingfds.push_back(client_sckt);
					connections[conn_fd] = new clientsockt(conn_fd);
					// listen_socket->clearMessage();
				}
				else
				{
					clientsockt *curr = connections[pollfds[i].fd];
					short re = pollfds[i].revents;
					if (re & POLLNVAL)
					{
						continue;
					}
					if (re & (POLLERR | POLLHUP))
					{
						//READ FIRST IF POLLIN OR HANG UP THE LINE
						continue;
					}
					if (re & POLLIN) // looking for reading
					{
						ssize_t n = curr->ft_recv();
						if(curr->getReadBuffer().size() > 0)
						{
							std::cout << "From client_fd: " << curr->getSocket_nbr() << "\n"  << curr->getReadBuffer();
							std::cout << "---------------" << std::endl;
						}

						if(n == 0)
							closing_fds.push_back(curr->getSocket_nbr());
						if (curr->getReadBuffer().find("\r\n\r\n") != std::string::npos)
						{
							curr->cleaReadBuffer();
							curr->setWriteBuffer(ft_make_dummyheader()); 
							pollfds[i].events = POLLOUT;  
						}
					}
					if (re & POLLOUT) // looking for writing
					{
						if(curr->ft_handleWrite())
							pollfds[i].events = POLLIN;
						else
							closing_fds.push_back(curr->getSocket_nbr());
					}
				}
			}
			for(size_t i = 0; i < closing_fds.size(); i++)
			{
				for (size_t j = 0; j < pollfds.size(); j++)
				{
					if (pollfds[j].fd == closing_fds[i])
					{ 
						pollfds.erase(pollfds.begin() + j); 
						break; 
					}
				} //TODO i dont like this solution maybe more practical one can done
				delete connections[closing_fds[i]];
				connections.erase(closing_fds[i]);
			}
			closing_fds.clear();
			for(size_t i = 0; i < pendingfds.size(); i++)
				pollfds.push_back(pendingfds[i]);
			pendingfds.clear();
		}

		delete(listen_socket);
		return 0;
	}
	catch(std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		delete(listen_socket);
		return 1;
	}
}