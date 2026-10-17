#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>

struct HttpResponse {
    int status;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string serialize(bool headOnly = false) const;
};

#endif