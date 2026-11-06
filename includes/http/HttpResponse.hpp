#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>
#include <vector>

struct HttpResponse {
    HttpResponse() : status(200) {}

    int status;
    std::map<std::string, std::string> headers;
    std::vector<std::string> set_cookies;
    std::string body;

    std::string serialize(bool headOnly = false) const;
};

#endif