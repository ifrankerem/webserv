#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <sys/types.h>


class HttpRequest
{
	private:
		int flag;
		int	status;
		std::map<std::string,std::string>	headers;
		std::string	body;
		size_t	bytesSent;

	public:
		enum	State
		{
			PARSE_OK,
			PARSE_ERROR,
			PARSE_INCOMPLETE
		};
		
	HttpRequest();
	~HttpRequest();
	HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
	void setByteSent(ssize_t n);
	State parse(const std::string& buf);
	bool is_complete(const std::string& buf,ssize_t nbr);
	void incrementFlag();
};

#endif