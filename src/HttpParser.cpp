#include "../includes/HttpParser.hpp"
#include <cstdlib>
#include <iostream>

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
        // Chunked Transfer Encoding decode
        else if (_request.headers.find("Transfer-Encoding") != _request.headers.end() &&
                 _request.headers["Transfer-Encoding"] == "chunked") {
            _state = PARSE_CHUNKED;
        }
        // GET/DELETE gibi gövdesi olmayan istekler direkt bitmiştir
        else {
            _state = PARSE_DONE;
        }
    }

    // --- 3. AŞAMA: CHUNKED BODY OKUMA ---
    while (_state == PARSE_CHUNKED) {
        // Chunk boyutu satırını bul (\r\n ile biter)
        size_t crlf_pos = _raw_data.find("\r\n");
        if (crlf_pos == std::string::npos)
            return; // Boyut satiri henuz tam gelmediyse bekle

        // Hex boyutu al
        std::string size_str = _raw_data.substr(0, crlf_pos);
        // Chunk extension varsa (boyut;name=value) sadece boyutu al
        size_t semi_pos = size_str.find(';');
        if (semi_pos != std::string::npos)
            size_str = size_str.substr(0, semi_pos);

        size_t chunk_size = 0;
        for (size_t k = 0; k < size_str.size(); k++) {
            char c = size_str[k];
            chunk_size *= 16;
            if (c >= '0' && c <= '9')      chunk_size += c - '0';
            else if (c >= 'a' && c <= 'f') chunk_size += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') chunk_size += c - 'A' + 10;
            else { _state = PARSE_ERROR; return; } // Geçersiz hex karakter
        }

        // Son chunk (0\r\n\r\n) -> okuma bitti
        if (chunk_size == 0) {
            _state = PARSE_DONE;
            return;
        }

        // Boyut satiri + \r\n + veri + \r\n kadar veri geldi mi?
        size_t needed = crlf_pos + 2 + chunk_size + 2;
        if (_raw_data.size() < needed)
            return; // Chunk'in tamami henuz gelmedi, bekle

        // Chunk verisini body'e ekle
        _request.body.append(_raw_data, crlf_pos + 2, chunk_size);

        // Okunan chunk'i (boyut satiri + veri + \r\n) buffer'dan sil
        _raw_data.erase(0, needed);
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

void HttpParser::printDebug() const {
    std::cout << "\n========== [ PARSER DEBUG ] ==========" << std::endl;
    std::cout << "Method  : [" << _request.method << "]" << std::endl;
    std::cout << "Path    : [" << _request.path << "]" << std::endl;
    std::cout << "Query   : [" << _request.query << "]" << std::endl;
    std::cout << "Version : [" << _request.version << "]" << std::endl;
    
    std::cout << "--- Headers (" << _request.headers.size() << ") ---" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin(); 
         it != _request.headers.end(); ++it) {
        std::cout << "  '" << it->first << "' : '" << it->second << "'" << std::endl;
    }
    
    std::cout << "--- Body (" << _request.body.length() << " bytes) ---" << std::endl;
    if (_request.body.empty())
        std::cout << "  (Body Bos)" << std::endl;
    else
        std::cout << _request.body << std::endl;
    std::cout << "======================================\n" << std::endl;
}