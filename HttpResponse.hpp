#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sys/types.h>


class HttpResponse
{

	private:
		int status_code;
		std::map<std::string,std::string>	headers;
		std::string	body;
		
	public:
	
	    HttpResponse();
	    HttpResponse(const HttpResponse& other);
	    HttpResponse& operator=(const HttpResponse& other);
	    ~HttpResponse();

    
};

#endif