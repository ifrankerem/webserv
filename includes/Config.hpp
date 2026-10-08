#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct Listen {
    std::string ip;
    int port;
};

struct LocationConfig {
    LocationConfig(): autoindex(false), upload_enable(false), client_max_body_size(0),redirect_code(0) {}

    std::string path;
    std::string root;
    std::string alias;
    std::vector<std::string> index;
    std::vector<std::string> allowed_methods;
    bool autoindex;
    bool upload_enable;
    size_t client_max_body_size;
    std::string upload_store;
    int redirect_code;
    std::string redirect_target;
    std::map<std::string, std::string> cgi_pass;
};

struct ServerConfig {
    ServerConfig() : client_max_body_size(0) {}

    std::vector<Listen> listens;
    std::string root;
    std::vector<std::string> server_name;
    std::vector<std::string> index;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
};

struct Config {
    std::vector<ServerConfig> servers;
};
    
#endif