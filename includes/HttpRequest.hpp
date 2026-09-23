#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

enum ParseState {
    PARSE_OK,
    PARSE_INCOMPLETE,
    PARSE_ERROR
};

struct HttpRequest {
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