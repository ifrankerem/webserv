#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <netinet/in.h>


class ServerConfig
{
	private:
		std::string host;
		int port;
		//TODO all elements from each {}
		// 
public:
	
    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();
    uint32_t getHost();
    uint16_t getPort(); //TODO these will be converting inside
    std::string getHost_str();
    int getPort_int();

    
};

#endif
