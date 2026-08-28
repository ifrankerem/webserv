#ifndef PARSING_HPP
#define PARSING_HPP

#include <string>

class parsing
{
	private:
		static std::string	toLower(const std::string& s);

	public:
		parsing();
		~parsing();
		parsing(const parsing& other);
		parsing& operator=(const parsing& other);

		static std::string	to_str(size_t n);
		static long	getContentLength(const std::string& raw);
};

#endif
