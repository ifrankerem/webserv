#include "../includes/http/HttpParser.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "--- Testing HttpParser ---" << std::endl;
    int errors = 0;

    HttpParser parser;
    std::string request = "GET /index.html?name=test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    
    // Ag baglantisi olmadan dogrudan string'i parser'a atiyoruz
    parser.feed(request.c_str(), request.length());
    parser.parse();

    if (parser.getState() != PARSE_DONE) {
        std::cerr << "FAIL: Parser durumu PARSE_DONE degil." << std::endl;
        errors++;
    }

    HttpRequest req = parser.getParsedRequest();
    if (req.method != "GET" || req.path != "/index.html" || req.query != "name=test") {
        std::cerr << "FAIL: Ayiklanan HTTP degerleri yanlis!" << std::endl;
        errors++;
    } else {
        //std::cout << "PASS: Basit GET istegi sorunsuz ayiklandi." << std::endl;
    }

    // Pipelining testi: iki istek tek feed ile atiliyor
    HttpParser pipe_parser;
    std::string pipe_req = "GET /ilk HTTP/1.1\r\nHost: localhost\r\n\r\nGET /ikinci HTTP/1.1\r\nHost: localhost\r\n\r\n";
    pipe_parser.feed(pipe_req.c_str(), pipe_req.length());
    pipe_parser.parse();

    if (pipe_parser.getState() != PARSE_DONE || pipe_parser.getParsedRequest().path != "/ilk") {
        std::cerr << "FAIL: Pipelining 1. istek ayrilamadi." << std::endl;
        errors++;
    }

    pipe_parser.reset();
    pipe_parser.parse();

    if (pipe_parser.getState() != PARSE_DONE || pipe_parser.getParsedRequest().path != "/ikinci") {
        std::cerr << "FAIL: Pipelining 2. istek ayrilamadi." << std::endl;
        errors++;
    }

    // Edge case 1: İki noktası olmayan başlık satırı (örn: connEctİon = clo se) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nconnEctİon = clo se\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: İki noktasiz header (connEctİon = clo se) PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 2: İki noktadan önce boşluk (RFC 7230 §3.2.4: Space before colon) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection : close\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: İki noktadan once bosluk (Connection : close) PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 3: Başlık isminde boşluk (Content Length: 10) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nContent Length: 10\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: Baslik isminde bosluk PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 4: Boş başlık ismi (: value) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n: value\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: Bos baslik ismi (: value) PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 5: Çift Host başlığı (RFC 7230 §5.4: Duplicate Host) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: a.com\r\nHost: b.com\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: Cift Host basligi PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 6: Request-Line başında boşluk ( GET / HTTP/1.1) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = " GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: Request-line basinda bosluk PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 7: Request-Line sonunda boşluk (GET / HTTP/1.1 ) -> PARSE_ERROR
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1 \r\nHost: localhost\r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_ERROR) {
            std::cerr << "FAIL: Request-line sonunda bosluk PARSE_ERROR olmaliydi!" << std::endl;
            errors++;
        }
    }

    // Edge case 8: Header değerinde baştaki ve sondaki boşluk/tab'ların trim edilmesi -> PARSE_DONE
    {
        HttpParser p;
        std::string req = "GET / HTTP/1.1\r\nHost: \t  localhost \t \r\nCustom:  val \r\n\r\n";
        p.feed(req.c_str(), req.length());
        p.parse();
        if (p.getState() != PARSE_DONE) {
            std::cerr << "FAIL: Header trim durumunda PARSE_DONE olmaliydi!" << std::endl;
            errors++;
        } else if (p.getParsedRequest().headers.at("host") != "localhost" ||
                   p.getParsedRequest().headers.at("custom") != "val") {
            std::cerr << "FAIL: Header degerleri dogru trimlenmedi!" << std::endl;
            errors++;
        }
    }

    if (errors > 0) {
        std::cerr << errors << " test(s) failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Tüm HttpParser testleri basariyla gecti." << std::endl;
    return 0;
}
