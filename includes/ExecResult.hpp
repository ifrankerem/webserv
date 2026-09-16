#ifndef EXEC_RESULT_HPP
#define EXEC_RESULT_HPP

#include "HttpResponse.hpp" 

class CgiProcess;

struct ExecResult {
    enum Kind {
        RESPONSE_READY,
        CGI_PENDING
    };

    Kind kind;
    HttpResponse response;
    CgiProcess* cgi; 
};

#endif