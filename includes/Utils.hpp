#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <map>
#include "HttpResponse.hpp"

namespace utils
{
    std::string toString(size_t n);
    std::string httpReason(int status);
    std::string getExtension(const std::string& path);
    std::string toLower(const std::string& s);
    std::string getHeader(const std::map<std::string, std::string>& headers, const std::string& key);
    HttpResponse defaultErrorPage(int status);
    std::vector<char*> mapToEnv(const std::map<std::string, std::string>& env);
}

#endif
