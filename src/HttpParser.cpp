#include "../includes/HttpParser.hpp"
#include <cstdlib>

// İlk yaratıldığında direkt okuma aşamasından başlatıyoruz
HttpParser::HttpParser() : _state(PARSE_READING) {}

HttpParser::~HttpParser() {}

void HttpParser::feed(const char* chunk, int size) {
    if (chunk && size > 0)
        _raw_data.append(chunk, size);
}

void HttpParser::parse() {
    // İşlem önceden bitmişse veya hata vermişse dur
    if (_state == PARSE_DONE || _state == PARSE_ERROR)
        return;

    // --- 1. AŞAMA: İLK SATIR VE BAŞLIKLARI (HEADERS) OKUMA ---
    while (_state == PARSE_READING) {
        // \r\n arıyoruz. Eğer yoksa, satır daha tam gelmemiştir, recv beklenir.
        size_t pos = _raw_data.find("\r\n");
        if (pos == std::string::npos) 
            return; 

        // Satırı havuzdan al ve asıl havuzdan temizle
        std::string line = _raw_data.substr(0, pos);
        _raw_data.erase(0, pos + 2); 

        // 1.1: Request Line okunmadıysa (Method hala boşsa)
        if (_request.method.empty()) {
            size_t first_space = line.find(' ');
            size_t last_space = line.rfind(' ');

            // Boşluk yoksa veya hatalı bir format geldiyse
            if (first_space == std::string::npos || first_space == last_space) {
                _state = PARSE_ERROR; 
                return;
            }

            _request.method = line.substr(0, first_space);
            _request.path = line.substr(first_space + 1, last_space - first_space - 1);
            _request.version = line.substr(last_space + 1);

            // Ekstra: Path içinde Query parametresi (?q=1) varsa ayır
            size_t query_pos = _request.path.find('?');
            if (query_pos != std::string::npos) {
                _request.query = _request.path.substr(query_pos + 1);
                _request.path = _request.path.substr(0, query_pos);
            }
        } 
        // 1.2: Satır tamamen boşsa (\r\n\r\n), başlıklar bitti gövdeye geç
        else if (line.empty()) {
            _state = PARSE_HEADERS_DONE;
            break; 
        } 
        // 1.3: Normal Başlık (Header) satırlarını ayıklama
        else {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                
                // Değerin başındaki gereksiz boşlukları temizle
                size_t first_non_space = value.find_first_not_of(" ");
                if (first_non_space != std::string::npos) {
                    value = value.substr(first_non_space);
                }
                _request.headers[key] = value;
            }
        }
    }

    // --- 2. AŞAMA: GÖVDE (BODY) OKUMA ---
    if (_state == PARSE_HEADERS_DONE) {
        // Eğer POST/PUT gibi Body içeren bir istek ise Content-Length olur
        if (_request.headers.find("Content-Length") != _request.headers.end()) {
            size_t content_length = std::atoi(_request.headers["Content-Length"].c_str());
            
            // Havuzdaki veri yeterliyse gövdeyi kopar ve işlemi bitir
            if (_raw_data.length() >= content_length) {
                _request.body = _raw_data.substr(0, content_length);
                _raw_data.erase(0, content_length);
                _state = PARSE_DONE;
            }
            // Veri yetersizse hiçbir şey yapma, sonraki recv() çağrısında tamamlanır.
        } 
        // Chunked encoding kuralı kontrolü (Basitlik için error'a düşürdük)
        else if (_request.headers.find("Transfer-Encoding") != _request.headers.end() && 
                 _request.headers["Transfer-Encoding"] == "chunked") {
            _state = PARSE_ERROR; 
        }
        // GET/DELETE gibi gövdesi olmayan istekler direkt bitmiştir
        else {
            _state = PARSE_DONE;
        }
    }
}

const HttpRequest& HttpParser::getParsedRequest() const {
    return _request;
}

ParseState HttpParser::getState() const {
    return _state;
}

void HttpParser::reset() {
    _raw_data.clear();
    _request = HttpRequest(); // Struct'ı tamamen sıfırlar
    _state = PARSE_READING;
}