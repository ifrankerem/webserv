#ifndef RESPONSE_HANDLER
#define RESPONSE_HANDLER

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"

class ResponseHandler
{
private:
	ResponseHandler(); //there will be no instance for this class


public:
    ResponseHandler(const ResponseHandler& other);
    ResponseHandler& operator=(const ResponseHandler& other);
    ~ResponseHandler();

    static HttpResponse redirect_handler(int status_code ,const std::string& target); //TODO
    static HttpResponse handle(const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
};

#endif
