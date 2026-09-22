#include "ResponseHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

ResponseHandler::ResponseHandler()
{
}

ResponseHandler::ResponseHandler(const ResponseHandler& other)
{
    *this = other;
}

ResponseHandler& ResponseHandler::operator=(const ResponseHandler& other)
{
    if (this != &other)
    {
        
    }

    return *this;
}

ResponseHandler::~ResponseHandler()
{
}

HttpResponse ResponseHandler::redirect_handler(int status_code,const std::string &target)
{
	HttpResponse response;
	response.status = status_code;
	response.headers["Location"] = target;
	return response;
}


HttpResponse ResponseHandler::handle(const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	if(req.method == "GET")
	{
		
	}
	else if(req.method == "POST")
	{
		
	}
	else if(req.method == "DELETE")
	{

	}

	//TODO B2/B3: fill the branches above; until then keep the build green
	(void)loc;
	(void)srv;
	HttpResponse response;
	response.status = 501;
	return response;
}

