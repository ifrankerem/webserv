#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "HttpRequest.hpp"

enum ParseState {
    PARSE_READING,      // Veri hala parça parça geliyor
    PARSE_HEADERS_DONE, // Başlıklar bitti, gövde okunuyor
    PARSE_CHUNKED,      // Transfer-Encoding: chunked, chunk'lar okunuyor
    PARSE_DONE,         // Her şey kusursuz bitti
    PARSE_ERROR         // Syntax hatası var
};

class HttpParser {
    private:
        std::string _raw_data; // recv'den gelen verilerin biriktiği havuz
        HttpRequest _request;  // Dolduracağımız struct kutusu              
        ParseState  _state;    // Parser'ın anlık durumu
        size_t      _total_body_read;
        size_t          _s_index;//yeni eklendi

    public:
        HttpParser();
        ~HttpParser();

        void feed(const char* chunk, int size);
        void parse();
        const HttpRequest& getParsedRequest() const;
        ParseState getState() const;
        HttpRequest& getParsedRequest();
        void reset(); // Yeni istek gelirse motoru temizlemek 
        size_t getTotalBodyRead() const;
        void printDebug() const;//bunu sil
};

#endif