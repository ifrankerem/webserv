#include "Utils.hpp"
#include <sstream>
#include <cctype>
#include <cstring>
#include <climits>
#include <netdb.h>

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
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "";

    size_t slash = path.rfind('/');
    if (slash != std::string::npos && dot < slash)
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

static int hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static std::string percentDecode(const std::string& s, bool plusIsSpace)
{
    std::string out;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            int high = hexValue(s[i + 1]);
            int low = hexValue(s[i + 2]);
            if (high != -1 && low != -1)
            {
                out += static_cast<char>(high * 16 + low);
                i += 2;
                continue;
            }
        }
        if (plusIsSpace && s[i] == '+')
            out += ' ';
        else
            out += s[i];
    }
    return out;
}

std::string urlDecode(const std::string& s)
{
    return percentDecode(s, true);
}

std::string decodePath(const std::string& s)
{
    return percentDecode(s, false);
}

bool endsWithSlash(const std::string& s)
{
    return !s.empty() && s[s.size() - 1] == '/';
}

std::string joinPath(std::string base, const std::string& rest)
{
    if (endsWithSlash(base))
        base.erase(base.size() - 1);

    bool restHasSlash = !rest.empty() && rest[0] == '/';
    if (restHasSlash)
        return base + rest;
    return base + "/" + rest;
}

std::string trim(const std::string& s)
{
    size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

std::string getHeader(const std::map<std::string, std::string>& headers, const std::string& key)
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it == headers.end())
        return "";
    return it->second;
}

HttpResponse defaultErrorPage(int status)
{
    std::string code = toString(static_cast<size_t>(status));

    HttpResponse x;
    x.status = status;
    x.headers["Content-Type"] = "text/html; charset=UTF-8";
    x.body = "<html>\n<head><title>" + code + "</title></head>\n"
             "<body>\n<center><h1>" + code + "</h1></center>\n"
             "<hr><center>webserv</center>\n</body>\n</html>\n";
    return x;
}

std::vector<std::string> mapToEnv(const std::map<std::string, std::string>& env)
{
    std::vector<std::string> out;
    std::map<std::string, std::string>::const_iterator it;
    for (it = env.begin(); it != env.end(); ++it)
        out.push_back(it->first + "=" + it->second);
    return out;
}

int atoi(const std::string &str)
{
    if (str.empty())
        return -1;

    long value = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;
        value = value * 10 + (str[i] - '0');
        if (value > INT_MAX)
            return -1;
    }
    return static_cast<int>(value);
}

in_addr_t inet_addr(const std::string& ip)
{
    struct addrinfo hints, *res = NULL;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(ip.c_str(), NULL, &hints, &res) != 0 || !res)
        return static_cast<in_addr_t>(-1);

    in_addr_t s_addr = reinterpret_cast<struct sockaddr_in*>(res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);
    return s_addr;
}
} // namespace utils
