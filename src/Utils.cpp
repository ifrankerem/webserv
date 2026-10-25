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

std::string urlDecode(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            int hi = 0;
            if (s[i + 1] >= '0' && s[i + 1] <= '9') hi = s[i + 1] - '0';
            else if (s[i + 1] >= 'a' && s[i + 1] <= 'f') hi = s[i + 1] - 'a' + 10;
            else if (s[i + 1] >= 'A' && s[i + 1] <= 'F') hi = s[i + 1] - 'A' + 10;
            else { out += s[i]; continue; }

            int lo = 0;
            if (s[i + 2] >= '0' && s[i + 2] <= '9') lo = s[i + 2] - '0';
            else if (s[i + 2] >= 'a' && s[i + 2] <= 'f') lo = s[i + 2] - 'a' + 10;
            else if (s[i + 2] >= 'A' && s[i + 2] <= 'F') lo = s[i + 2] - 'A' + 10;
            else { out += s[i]; continue; }

            out += static_cast<char>((hi << 4) | lo);
            i += 2;
        }
        else if (s[i] == '+')
            out += ' ';
        else
            out += s[i];
    }
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

int atoi(const std::string &str)
{
    int i = 0;
    long val = 0;

    if (str.empty())
        return -1;

    while (str[i])
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            if (val > 214748364 || (val == 214748364 && (str[i] - '0') > 7))
                return -1;
            val = (str[i] - 48) + (10 * val);
        }
        else
            return -1;
        i++;
    }
    return (int)val;
}
} // namespace utils
