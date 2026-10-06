#include "../includes/Utils.hpp"
#include <sstream>
#include <cctype>
#include <cstring>

namespace utils
{

std::string toString(size_t n)
{
    std::ostringstream out;
    out << n;
    return out.str();
}

struct ReasonEntry
{
    int         status;
    const char* text;
};

static const ReasonEntry reasonTable[] = {
    { 200, "OK" },
    { 201, "Created" },
    { 204, "No Content" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 303, "See Other" },
    { 307, "Temporary Redirect" },
    { 308, "Permanent Redirect" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 408, "Request Timeout" },
    { 409, "Conflict" },
    { 411, "Length Required" },
    { 413, "Payload Too Large" },
    { 414, "URI Too Long" },
    { 415, "Unsupported Media Type" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Unavailable" },
    { 504, "Gateway Timeout" },
    { 505, "HTTP Version Not Supported" }
};

std::string httpReason(int status)
{
    size_t count = sizeof(reasonTable) / sizeof(reasonTable[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (reasonTable[i].status == status)
            return reasonTable[i].text;
    }
    return "Unknown";
}

std::string getExtension(const std::string& path)
{
    size_t slash = path.rfind('/');
    size_t dot   = path.rfind('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return "";
    return path.substr(dot);
}

std::string toLower(const std::string& s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string getHeader(const std::map<std::string, std::string>& headers, const std::string& key)
{
    std::string lowerKey = toLower(key);
    std::map<std::string, std::string>::const_iterator it = headers.begin();
    while (it != headers.end())
    {
        if (toLower(it->first) == lowerKey)
            return it->second;
        ++it;
    }
    return "";
}

HttpResponse defaultErrorPage(int status)
{
    HttpResponse x;
    x.status = status;
    x.headers["Content-Type"] = "text/html; charset=UTF-8";
    x.body =
        "<html>\n<head><title>" + toString(static_cast<size_t>(status)) + "</title></head>\n"
        "<body>\n<center><h1>" + toString(static_cast<size_t>(status)) + "</h1></center>\n"
        "<hr><center>webserv</center>\n</body>\n</html>\n";
    return x;
}

std::vector<char*> mapToEnv(const std::map<std::string, std::string>& env)
{
    std::vector<char*> out;
    std::map<std::string, std::string>::const_iterator it = env.begin();
    while (it != env.end())
    {
        std::string pair = it->first + "=" + it->second;
        out.push_back(::strdup(pair.c_str()));
        ++it;
    }
    out.push_back(NULL);
    return out;
}

} // namespace utils
