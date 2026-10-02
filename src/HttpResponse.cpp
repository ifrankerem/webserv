#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <cstddef>

std::string HttpResponse::serialize() const
{
	std::string out;

	out += "HTTP/1.1 ";
	out += utils::toString(static_cast<size_t>(status));
	out += " ";
	out += utils::httpReason(status);
	out += "\r\n";

	std::map<std::string, std::string> all = headers;
	all["Content-Length"] = utils::toString(body.size());
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
