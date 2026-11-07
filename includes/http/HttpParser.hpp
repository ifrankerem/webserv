#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "HttpRequest.hpp"

enum ParseState 
{
    PARSE_READING,
    PARSE_HEADERS_DONE,
    PARSE_CHUNKED,
    PARSE_DONE,
    PARSE_ERROR
};

class HttpParser 
{
    private:
        std::string _raw_data;
        HttpRequest _request;             
        ParseState  _state;
        size_t      _total_body_read;
        size_t      _s_index;
    public:
        HttpParser();

        void                feed(const char* chunk, int size);
        void                parse();
        const HttpRequest&  getParsedRequest() const;
        ParseState          getState() const;
        HttpRequest&        getParsedRequest();
        void                reset(); 
        size_t              getTotalBodyRead() const;
};

#endif