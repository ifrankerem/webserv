#include "parsing.hpp"
#include <sstream>
#include <cctype>
#include <cstdlib>

parsing::parsing()
{
}

parsing::~parsing()
{
}

parsing::parsing(const parsing& other)
{
	*this = other;
}

parsing& parsing::operator=(const parsing& other)
{
	(void)other;
	return (*this);
}

std::string parsing::toLower(const std::string& s)
{
	std::string out = s;

	for (std::string::size_type i = 0; i < out.size(); ++i)
		out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
	return (out);
}

std::string parsing::to_str(size_t n)
{
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

long parsing::getContentLength(const std::string& raw)
{
	const std::string key = "content-length:";
	std::string::size_type pos = toLower(raw).find(key);
	if (pos == std::string::npos)
		return (-1);

	return (std::atol(raw.c_str() + pos + key.size()));
}
