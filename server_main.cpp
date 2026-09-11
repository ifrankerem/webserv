#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "listensockt.hpp"
#include "HttpRequest.hpp"
#include "clientsockt.hpp"
#include <csignal>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <vector>
#include <map>
#include <poll.h>
#include "Config.hpp"




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
	std::ostringstream oss;
	oss << content.length();
	std::string content_length_str = oss.str(); // FIXME REMOVE LATER IT BECAUSE ITS NOT CPP 98 !!!!! 
	std::string dummy_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
	content_length_text.append(content_length_str);
	dummy_header.append(content_length_text);
	dummy_header.append("\r\n\r\n");
	dummy_header.append(content);
	return dummy_header;
}

int main()
{
	Config config;
	std::map<int,ServerConfig> listen_sockets;
	std::vector <struct pollfd> pollfds;
	std::vector <struct pollfd> pendingfds;
	std::map<int,clientsockt*> connections;
	std::vector <int> closing_fds;
	std::vector<ServerConfig> servers_list = config.getServers(); //INFO gettin serverslist

	try{
		signal(SIGPIPE, SIG_IGN); //TODO simdilik ekliyorum daha detayli arastirmasini yapicam


		for(int i = 0; i < config.getNbr_of_UniquePairs(); i++)
		{
			listensockt *listen_socket = new listensockt(AF_INET,SOCK_STREAM,0);
			uint16_t port = servers_list[i].getPort();
			uint32_t host = servers_list[i].getHost();
			//INFO where i can connect with that socket
			listen_socket->set_Host(servers_list[i].getHost_str());
			listen_socket->setPort(servers_list[i].getPort_int());
			listen_socket->init_addr(AF_INET,port,host); //INFO 127.0.0.1 for testing
			listen_socket->ft_bind();
			listen_socket->ft_listen();
			struct pollfd pfd = pollfd(); //INFO no memset so recreate the struct
			pfd.fd = listen_socket->getSocket_nbr(); //INFO adding listen sockt to q
			pfd.events = POLLIN;
			pollfds.push_back(pfd);
			listen_sockets[listen_socket->getSocket_nbr()] = servers_list[i];
		}


		while(1)
		{ //TODO connect to listen fds to this loop
			poll(&pollfds[0],pollfds.size(),-1); //TODO Subject: "A request to your server should never hang indefinitely."
			for(size_t i = 0; i < pollfds.size();i++)
			{
				if(pollfds[i].revents == 0)
					continue; //nothing happens in this socket
				if(pollfds[i].fd == listen_socket->getSocket_nbr()) //INFO this is the listen_fd
				{
					int conn_fd = listen_socket->ft_accept();
					if(conn_fd == -1)
						continue; //this connection cannot made it so continue
					struct pollfd client_sckt = pollfd(); //INFO no memset so recreate the struct
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
					if (re & POLLIN) //INFO looking for reading
					{
						HttpRequest &request = curr->getRequest();
						request.incrementFlag();
						ssize_t prev = curr->getReadBuffer().size();
						ssize_t n = curr->ft_recv();
						request.setByteSent(n);
						if(n == 0)
							closing_fds.push_back(curr->getSocket_nbr());
						if(request.is_complete(curr->getReadBuffer(),prev) == true)
						{
							switch (request.parse(curr->getReadBuffer()))
							{
								case HttpRequest::PARSE_INCOMPLETE:
									continue; //INFO waiting for more data, next fd
								case HttpRequest::PARSE_ERROR:
									closing_fds.push_back(curr->getSocket_nbr()); //NOTE closing fd for errors
									break;
								case HttpRequest::PARSE_OK:
									// INFO Making preparetment for RESPONSE
									HttpResponse &response = curr->getResponse();
									std::string dummy_config; // TODO there will be a config file while webserv started
									response.build(request, dummy_config);
									curr->clearReadBuffer();
									curr->setWriteBuffer(response.ft_make_response());
									pollfds[i].events = POLLOUT;
									break;
							}
						}
					}
					if (re & POLLOUT) //INFO looking for writing
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
				} //FIXME i dont like this solution maybe more practical one can done
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