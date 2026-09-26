#ifndef RESPONSE_HANDLER
#define RESPONSE_HANDLER

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <dirent.h>

class ResponseHandler
{
private:
	ResponseHandler(); //there will be no instance for this class


public:
    ResponseHandler(const ResponseHandler& other);
    ResponseHandler& operator=(const ResponseHandler& other);
    ~ResponseHandler();

    static HttpResponse redirect_handler(int status_code ,const std::string& target); //TODO
    static HttpResponse handle(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse handle_get(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse handle_post(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse handle_delete(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse serveFile(const std::string diskPath,const ServerConfig& srv);
    static HttpResponse listDirectory(const std::string diskPath,const ServerConfig& srv);


};

#endif
