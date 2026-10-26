#include "../includes/HttpParser.hpp"
#include "../includes/Utils.hpp"
#include <cstdlib>
#include <iostream>

HttpParser::HttpParser() : _state(PARSE_READING), _total_body_read(0), _s_index(0){}

HttpParser::~HttpParser() {}

void HttpParser::feed(const char* chunk, int size) {
    if (chunk && size > 0)
        _raw_data.append(chunk, size);
}

void HttpParser::parse() {
    if (_state == PARSE_DONE || _state == PARSE_ERROR)
        return;

    while (_state == PARSE_READING) {
        size_t pos = _raw_data.find("\r\n", _s_index);
        if (pos == std::string::npos) 
            return; 

        std::string line = _raw_data.substr(_s_index, pos - _s_index);
        _s_index = pos + 2;

        if (_request.method.empty()) {
            size_t first_space = line.find(' ');
            size_t last_space = line.rfind(' ');

            if (first_space == std::string::npos || first_space == last_space) {
                _state = PARSE_ERROR; 
                return;
            }

            _request.method = line.substr(0, first_space);
            _request.path = line.substr(first_space + 1, last_space - first_space - 1);
            _request.version = line.substr(last_space + 1);

            size_t query_pos = _request.path.find('?');
            if (query_pos != std::string::npos) {
                _request.query = _request.path.substr(query_pos + 1);
                _request.path = _request.path.substr(0, query_pos);
            }
        } 
        else if (line.empty()) {
            _state = PARSE_HEADERS_DONE;
            _raw_data.erase(0, _s_index);
            _s_index = 0;
            break; 
        } 
        else {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = utils::toLower(line.substr(0, colon_pos));
                std::string value = line.substr(colon_pos + 1);
                
                size_t first_non_space = value.find_first_not_of(" ");
                if (first_non_space != std::string::npos) {
                    value = value.substr(first_non_space);
                }
                _request.headers[key] = value;
            }
        }
    }

    if (_state == PARSE_HEADERS_DONE) {
        if (_request.headers.find("content-length") != _request.headers.end()) {
            size_t content_length = std::atoi(_request.headers["content-length"].c_str());

            if (_raw_data.length() >= content_length) {
                if (_raw_data.length() == content_length) {
                    _request.body.swap(_raw_data);
                } else {
                    _request.body.assign(_raw_data, 0, content_length);
                    _raw_data.erase(0, content_length);
                }
                _state = PARSE_DONE;
            }
        } 
        else if (_request.headers.find("transfer-encoding") != _request.headers.end() &&
                 utils::toLower(_request.headers["transfer-encoding"]) == "chunked") {
            _state = PARSE_CHUNKED;
        }
        else {
            _state = PARSE_DONE;
        }
    }

    while (_state == PARSE_CHUNKED) {
        size_t crlf_pos = _raw_data.find("\r\n");
        if (crlf_pos == std::string::npos)
            return;

        std::string size_str = _raw_data.substr(0, crlf_pos);
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

        if (chunk_size == 0) {
            if (_raw_data.size() >= 5 && _raw_data.substr(0, 5) == "0\r\n\r\n") {
                _state = PARSE_DONE;
                _raw_data.erase(0, 5); 
                return;
            } else {
                return; 
            }
        }
        size_t needed = crlf_pos + 2 + chunk_size + 2;
        if (_raw_data.size() < needed)
            return; 

        _request.body.append(_raw_data, crlf_pos + 2, chunk_size);

        _total_body_read += chunk_size;
        _raw_data.erase(0, needed);
    }
}


const HttpRequest& HttpParser::getParsedRequest() const {
    return _request;
}

HttpRequest& HttpParser::getParsedRequest() {
    return _request;
}

ParseState HttpParser::getState() const {
    return _state;
}

void HttpParser::reset() {
    if (_raw_data.capacity() > 64 * 1024) {
        std::string().swap(_raw_data);
    } else {
        _raw_data.clear();
    }
    _total_body_read = 0;
    _s_index = 0;
    _request = HttpRequest(); // Struct'ı tamamen sıfırlar
    _state = PARSE_READING;
}

size_t HttpParser::getTotalBodyRead() const {
    return _total_body_read;
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