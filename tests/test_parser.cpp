#include "../includes/HttpParser.hpp"
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
        std::cout << "PASS: Basit GET istegi sorunsuz ayiklandi." << std::endl;
    }

    // Ekstra: Parcali veri (Chunk) feed testi eklenebilir.

    if (errors > 0) {
        std::cerr << errors << " test(s) failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Tüm HttpParser testleri basariyla gecti." << std::endl;
    return 0;
}
