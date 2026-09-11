#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpRequest.hpp"
#include <string>
#include <map>
#include <sys/types.h>


class HttpResponse
{

	private:
		int status_code;
		std::map<std::string,std::string> headers;
		std::string	body;
		
	public:
	
	    HttpResponse();
	    HttpResponse(const HttpResponse& other);
	    HttpResponse& operator=(const HttpResponse& other);
	    ~HttpResponse();
		void build(HttpRequest request, std::string config);
		std::string ft_make_response(); //TODO

    
};

#endif