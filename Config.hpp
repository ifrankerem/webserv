#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerConfig.hpp"
#include <vector>

class Config
{
	private:
		std::vector<ServerConfig> servers;

	public:
	    Config();
	    Config(const Config& other);
	    Config& operator=(const Config& other);
	    ~Config();
		int getNbr_of_UniquePairs();
	  	const std::vector<ServerConfig>& getServers() const; 

    
};

#endif
