#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

struct WatchFd { //CGI
    int   fd;
    short events;
};

struct Step {
	
    enum Kind {
        RESPOND,
        WAIT
    };

    Kind                 kind;
    HttpResponse         response;
    std::vector<WatchFd> watch;
};

class RequestHandler {

private:

	const ServerConfig* server;

	// CGI state
	bool        cgiActive;
	pid_t       cgiPid;
	int         cgiPipeFd;
	std::string cgiBuffer;

	const LocationConfig* findLocation(const std::string& path);
	bool            normalizePath(const std::string& path, std::string& out); //NOTE A decoded sekilde path vermeli bana!
	std::string     resolveDiskPath(const LocationConfig& loc, const std::string& path);
	std::string     resolveUploadPath(const LocationConfig& loc, const std::string& path);
	RequestHandler(const RequestHandler& other);
	RequestHandler& operator=(const RequestHandler& other);
	
public:
	
    RequestHandler(const ServerConfig& server);
    ~RequestHandler();
    size_t       bodyLimit(const HttpRequest& req);
    Step         handle(const HttpRequest& req);
    Step         onReady(int fd, short revents);
    static HttpResponse error(int status,const ServerConfig *server);

};

#endif
