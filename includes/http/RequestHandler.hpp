#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <cstddef>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
#include "../core/Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

struct WatchFd { //CGI
    int   fd;
    uint32_t events;
};

struct Step {
	
    enum Kind {
        RESPOND,
        WAIT
    };

    Kind                 kind;

    Step() : kind(RESPOND) {}
    HttpResponse         response;
    std::vector<WatchFd> watch;
    std::vector<int>     unwatch; // epoll'den cikarilacak fd'ler (kapanan CGI stdin)
};

class RequestHandler {

private:

	const ServerConfig* server;

	// CGI state
	bool        cgiActive;
	pid_t       cgiPid;
	int         cgiPipeFd;   // CGI stdout (parent okur)
	int         cgiStdinFd;  // CGI stdin (parent yazar), -1 = kapali
	std::string cgiBody;     // gövde, olay döngüsünde parça parça yazilir
	size_t      cgiInOffset;
	std::string cgiBuffer;

	const LocationConfig* findLocation(const std::string& path);
	bool            normalizePath(const std::string& path, std::string& out); //NOTE A decoded sekilde path vermeli bana!
	std::string     resolveDiskPath(const LocationConfig& loc, const std::string& path);
	std::string     resolveUploadPath(const LocationConfig& loc, const std::string& path);
	bool            validateRequest(HttpRequest& req, std::string& path, Step& x);
	const LocationConfig* applyLocationRules(HttpRequest& req, const std::string& path, Step& x);
	Step            execute(HttpRequest& req, const std::string& path, const LocationConfig* best);
	Step            executeCgi(HttpRequest& req, const std::string& path, const LocationConfig* best, const std::string& interpreter);
	Step            executeMethod(HttpRequest& req, const std::string& path, const LocationConfig* best);
	RequestHandler(const RequestHandler& other);
	RequestHandler& operator=(const RequestHandler& other);
	
public:
	
    RequestHandler(const ServerConfig& server);
    ~RequestHandler();
    size_t       bodyLimit(const HttpRequest& req);
    Step         handle(HttpRequest& req);
    void         closeCgiStdin(Step& x);
    Step         onReady(int fd, uint32_t revents);
    static HttpResponse error(int status,const ServerConfig *server);

};

#endif
