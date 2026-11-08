#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include "../http/HttpResponse.hpp"

namespace utils
{
    std::string toString(size_t n);
    std::string httpReason(int status);
    std::string getExtension(const std::string& path);
    std::string mimeType(const std::string& path);
    std::string toLower(const std::string& s);
    int hexValue(char c);
    std::string decodePath(const std::string& s);
    bool endsWithSlash(const std::string& s);
    std::string joinPath(std::string base, const std::string& rest);
    std::string trim(const std::string& s);
    std::string getHeader(const std::map<std::string, std::string>& headers, const std::string& key);
    HttpResponse defaultErrorPage(int status);
    std::vector<std::string> mapToEnv(const std::map<std::string, std::string>& env);
    int atoi(const std::string &str);
    bool parseSize(const std::string &str, size_t &out);
    in_addr_t inet_addr(const std::string& ip);
}

#endif
