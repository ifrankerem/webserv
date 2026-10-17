#include "../includes/HttpResponse.hpp"
#include "../includes/Utils.hpp"
#include <cstddef>

// headOnly: HEAD istegi. Content-Length GET'teki gibi gercek boyu soyler, govde yazilmaz (RFC 9110 8.6)
std::string HttpResponse::serialize(bool headOnly) const
{
	std::string out;
	// govde tek seferde sigsin: buyurken kapasite ikiye katlanip gecici kopya olusmasin
	out.reserve(body.size() + 1024);

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
	if (!headOnly)
		out += body;

	return out;
}
