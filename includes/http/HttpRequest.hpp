#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

struct HttpRequest {
    HttpRequest() : server_port(0) {}

    std::string method;
    std::string path;
    std::string query;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string remote_addr;
    int server_port;
};

#endif