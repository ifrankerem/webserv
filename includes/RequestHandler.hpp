#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <cstddef>
#include <vector>
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

// A'nin poll'da izlemesi gereken fd.
struct WatchFd {
    int   fd;
    short events;
};

// handle() ve onReady() donusu.
struct Step {
    enum Kind {
        RESPOND,
        WAIT
    };

    Kind                 kind;
    HttpResponse         response;
    std::vector<WatchFd> watch;
};

// A ile B arasindaki tek kapi. B yazar, A kullanir.
// Her istek icin bir tane; A olusturur, A siler.
class RequestHandler {
public:
    RequestHandler(const ServerConfig& server);
    ~RequestHandler();

    size_t       bodyLimit(const HttpRequest& req);
    Step         handle(const HttpRequest& req);
    Step         onReady(int fd, short revents);
    HttpResponse error(int status);

private:
    RequestHandler(const RequestHandler& other);
    RequestHandler& operator=(const RequestHandler& other);
};

#endif
