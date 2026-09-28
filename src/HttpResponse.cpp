#include "HttpResponse.hpp"
#include <cstddef>
#include <sstream>

struct ReasonEntry {
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

static std::string reasonPhrase(int status)
{
	size_t count = sizeof(reasonTable) / sizeof(reasonTable[0]);
	for (size_t i = 0; i < count; i++)
	{
		if (reasonTable[i].status == status)
			return reasonTable[i].text;
	}
	return "Unknown";
}

static std::string toString(size_t n)
{
	std::ostringstream out;
	out << n;
	return out.str();
}

std::string HttpResponse::serialize() const
{
	std::string out;

	out += "HTTP/1.1 ";
	out += toString(static_cast<size_t>(status));
	out += " ";
	out += reasonPhrase(status);
	out += "\r\n";

	std::map<std::string, std::string> all = headers;
	all["Content-Length"] = toString(body.size());
	if (all.find("Server") == all.end())
		all["Server"] = "webserv";

	std::map<std::string, std::string>::const_iterator it = all.begin();
	while (it != all.end())
	{
		out += it->first;
		out += ": ";
		out += it->second;
		out += "\r\n";
		++it;
	}

	out += "\r\n";
	out += body;

	return out;
}
