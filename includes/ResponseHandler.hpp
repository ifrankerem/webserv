#ifndef RESPONSE_HANDLER
#define RESPONSE_HANDLER

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <dirent.h>
#include <map>
#include <string>

class ResponseHandler
{
private:
	ResponseHandler(); //there will be no instance for this class
	ResponseHandler(const ResponseHandler& other);
	ResponseHandler& operator=(const ResponseHandler& other);

public:
    ~ResponseHandler();

    static HttpResponse redirect_handler(int status_code ,const std::string& target); //TODO
    static HttpResponse handle(const std::string& diskPath,const std::string& path,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse handle_get(const std::string& diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv,bool head_flag);
    static HttpResponse handle_post(const std::string& diskPath,const std::string& path,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse handle_delete(const std::string& diskPath,const LocationConfig& loc,const ServerConfig& srv);
    static HttpResponse serveFile(const std::string& diskPath,const ServerConfig& srv,bool head_flag);
    static HttpResponse listDirectory(const std::string& diskPath, const LocationConfig& loc, const ServerConfig& srv);
    static void handle_cgi_child(const int output_fd,const int input_fd,std::map<std::string, std::string>& env_variables,const std::string& interpreter);


};

#endif
