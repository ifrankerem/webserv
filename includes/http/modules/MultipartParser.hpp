#ifndef MULTIPART_PARSER_HPP
#define MULTIPART_PARSER_HPP

#include <string>
#include <vector>

struct MultipartPart {
    std::string name;       // form-data field adi
    std::string filename;   // bos ise düz metin field (dosya degil)
    std::string contentType;
    std::string content;    // ham baytlar (ikili dosya guvenli)
};

// RFC 2046 multipart/form-data govdesini parcalara ayirir.
// Boundary yoksa veya govde bozuksa false doner.
bool parseMultipart(const std::string& body, const std::string& contentType,
                    std::vector<MultipartPart>& out);

#endif
