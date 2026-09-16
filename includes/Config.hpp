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
    std::string path;
    std::string root;
    std::string index;
    std::vector<std::string> allowed_methods;
    bool autoindex;
    size_t client_max_body_size;
    std::string upload_store;
    int redirect_code;
    std::string redirect_target;
    std::map<std::string, std::string> cgi_pass;
};

struct ServerConfig {
    std::vector<Listen> listens;
    std::string root;
    std::string index;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
};

struct Config {
    std::vector<ServerConfig> servers;
};

#endif