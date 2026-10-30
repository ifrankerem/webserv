#include "MultipartParser.hpp"
#include "Utils.hpp"
#include <cctype>

// header satirindan name="x" / filename="y" parametresinin degerini cikarir.
// lower: satirin kucuk harfli kopyasi (case-insensitive arama icin)
static std::string getParam(const std::string& line, const std::string& lower,
                            const std::string& param)
{
    size_t pos = lower.find(param + "=\"");
    // "filename=\"x\"" icinde "name=\"x\"" de gecbilir; onceki harf
    // alfanumerikse bu baska bir parametrenin sonu, atla.
    while (pos != std::string::npos && pos > 0 && std::isalnum(lower[pos - 1]))
        pos = lower.find(param + "=\"", pos + 1);
    if (pos == std::string::npos)
        return "";

    pos += param.size() + 2;
    size_t end = line.find('"', pos);
    if (end == std::string::npos)
        return "";
    return line.substr(pos, end - pos);
}

bool parseMultipart(const std::string& body, const std::string& contentType,
                    std::vector<MultipartPart>& out)
{
    out.clear();

    // Content-Type'tan boundary parametresini al (tirnak icine alinmis olabilir)
    std::string lower_type = utils::toLower(contentType);
    size_t bpos = lower_type.find("boundary=");
    if (bpos == std::string::npos)
        return false;

    std::string boundary = contentType.substr(bpos + 9);
    size_t semi = boundary.find(';');
    if (semi != std::string::npos)
        boundary = boundary.substr(0, semi);
    if (!boundary.empty() && boundary[0] == '"')
    {
        size_t end = boundary.find('"', 1);
        if (end == std::string::npos)
            boundary = boundary.substr(1);
        else
            boundary = boundary.substr(1, end - 1);
    }
    if (boundary.empty())
        return false;

    std::string delim = "--" + boundary;
    size_t pos = body.find(delim);
    if (pos == std::string::npos)
        return false;

    while (true)
    {
        size_t start = pos + delim.size();
        // kapanis ayraci: --boundary--
        if (body.compare(start, 2, "--") == 0)
            break;
        if (body.compare(start, 2, "\r\n") != 0)
            return false;
        start += 2;

        size_t next = body.find("\r\n" + delim, start);
        if (next == std::string::npos)
            return false;

        std::string part = body.substr(start, next - start);
        size_t hdr_end = part.find("\r\n\r\n");
        if (hdr_end == std::string::npos)
            return false;

        std::string headers = part.substr(0, hdr_end);
        std::string lower_headers = utils::toLower(headers);

        MultipartPart item;
        size_t cd = lower_headers.find("content-disposition:");
        if (cd != std::string::npos)
        {
            size_t cd_end = lower_headers.find("\r\n", cd);
            if (cd_end == std::string::npos)
                cd_end = headers.size();
            std::string cd_line = headers.substr(cd, cd_end - cd);
            std::string cd_lower = lower_headers.substr(cd, cd_end - cd);
            item.name = getParam(cd_line, cd_lower, "name");
            item.filename = getParam(cd_line, cd_lower, "filename");
        }

        size_t ct = lower_headers.find("content-type:");
        if (ct != std::string::npos)
        {
            size_t ct_end = lower_headers.find("\r\n", ct);
            if (ct_end == std::string::npos)
                ct_end = headers.size();
            item.contentType = utils::trim(headers.substr(ct + 13, ct_end - (ct + 13)));
        }

        item.content = part.substr(hdr_end + 4);
        out.push_back(item);
        pos = next + 2; // CRLF'i atla, delim'in "--"sinda kal
    }
    return true;
}
