#include "../includes/ConfigParser.hpp"
#include "../includes/Utils.hpp"

void ConfigParser::parse_location(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    LocationConfig loc;
    loc.autoindex = false; 
    loc.redirect_code = 0;
    loc.upload_enable = false;
    
    i++; 
    loc.path = tokens[i];
    
    i++;
    if (tokens[i] != "{") throw std::runtime_error("Config Error: Expected '{' after location path.");
    
    i++; 
    
    while (i < tokens.size() && tokens[i] != "}") {
        std::string cmd = tokens[i];
        
        if (location_command_map.find(cmd) != location_command_map.end()) {

            if (i + 1 >= tokens.size() || tokens[i + 1] == ";" || tokens[i + 1] == "{" || tokens[i + 1] == "}") 
                throw std::runtime_error("Config Error: Directive '" + cmd + "' requires at least one argument.");
            
            (this->*location_command_map[cmd])(tokens, i, loc);
        } else {
            throw std::runtime_error("Config Error: Unknown location directive '" + cmd + "'");
        }
        
        i++; 
    }
    
    if (i >= tokens.size() || tokens[i] != "}") throw std::runtime_error("Config Error: Missing '}' in location block.");
    
    server.locations.push_back(loc);
}

void ConfigParser::parse_autoindex(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    if (tokens[i] == "on")
        location.autoindex = true;
    else if (tokens[i] == "off") 
        location.autoindex = false;
    else 
        throw std::runtime_error("Config Error: 'autoindex' can only be 'on' or 'off'! Received value: " + tokens[i]);
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'autoindex' must end with ';'");
}

void ConfigParser::parse_upload_enable(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    if (tokens[i] == "on")
        location.upload_enable = true;
    else if (tokens[i] == "off") 
        location.upload_enable = false;
    else 
        throw std::runtime_error("Config Error: 'upload_enable' can only be 'on' or 'off'! Received value: " + tokens[i]);
    i++;
    if (tokens[i] != ";") throw std::runtime_error("Config Error: 'upload_enable' must end with ';'");
}

void ConfigParser::parse_location_root(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    location.root = tokens[i];
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: location 'root' must end with ';'");
}

void ConfigParser::parse_upload_store(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    location.upload_store = tokens[i];
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'upload_store' must end with ';'");
}

void ConfigParser::parse_cgi_path(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    std::string path = tokens[i];

    if (location.cgi_pass.size() > 0) { //bu kısımda opsiyon 1 seçeneğimiz ddaha var belki değiştiriebilir;
        location.cgi_pass.begin()->second = path;
    } else {
        location.cgi_pass[""] = path; 
    }
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'cgi_path' must end with ';'");
}

void ConfigParser::parse_cgi_ext(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    std::string ext = tokens[i];

    if (location.cgi_pass.count("")) {
        location.cgi_pass[ext] = location.cgi_pass[""];
        location.cgi_pass.erase("");
    } else {
        location.cgi_pass[ext] = "";
    }
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'cgi_ext' must end with ';'");
}

void ConfigParser::parse_location_index(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    while (i < tokens.size() && tokens[i] != ";") {

        if (tokens[i] == "{" || tokens[i] == "}") 
            throw std::runtime_error("Config Error: 'index' komutunda ';' unutulmus olabilir.");

        if (!location.index.empty()) location.index += " ";
        location.index += tokens[i];
        i++;
    }
   if (i >= tokens.size() || tokens[i] != ";") 
        throw std::runtime_error("Config Error: location 'index' must end with ';'");
}

void ConfigParser::parse_allow_methods(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    while (i < tokens.size() && tokens[i] != ";") {
        if (tokens[i] == "GET" || tokens[i] == "POST" || tokens[i] == "DELETE")
        {
            for (size_t j = 0; j < location.allowed_methods.size(); j++)
            {
                if (location.allowed_methods[j] == tokens[i])
                    throw std::runtime_error("Config Error: Duplicate method! '" + tokens[i] + "' is already added.");//break de atılabilir bilemedim
            }
            location.allowed_methods.push_back(tokens[i]);
        }
        else
            throw std::runtime_error("Config Error: Invalid method or missing ';' -> '" + tokens[i] + "'");
        i++;
    }
    if (i >= tokens.size() || tokens[i] != ";") 
        throw std::runtime_error("Config Error: 'allow_methods' directive must end with ';'");
}

void ConfigParser::parse_return(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
    i++;
    location.redirect_code = utils::atoi(tokens[i].c_str());//kendi atoini yaz hatalı bura 301kamil
    if (location.redirect_code < 100 || location.redirect_code > 599) //burayı sen düzeltirsin ben tam aralığını bilmiyorum zaten harf gelirse 0 olucak 0 diye durum kodu yok yine hata
        throw std::runtime_error("Config Error: Status code 'return' must be a number only!Received:");
    i++;
    if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
        throw std::runtime_error("Config Error: 'return' is expecting a directory path.");
    location.redirect_target = tokens[i];
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'return' must end with ';'");
}

