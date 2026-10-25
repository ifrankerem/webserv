#include "../includes/ConfigParser.hpp"
#include "../includes/Utils.hpp"      

ConfigParser::ConfigParser()
{
    command_map["listen"]               = &ConfigParser::parse_listen;
    command_map["server_name"]          = &ConfigParser::parse_server_name;
    command_map["root"]                 = &ConfigParser::parse_root;
    command_map["error_page"]           = &ConfigParser::parse_error_page;
    command_map["client_max_body_size"] = &ConfigParser::parse_client_max_body_size;
    command_map["index"]                = &ConfigParser::parse_index;
    command_map["location"]             = &ConfigParser::parse_location;

    location_command_map["root"]          = &ConfigParser::parse_location_root;
    location_command_map["alias"]         = &ConfigParser::parse_location_alias;
    location_command_map["index"]         = &ConfigParser::parse_location_index;
    location_command_map["client_max_body_size"] = &ConfigParser::parse_location_client_max_body_size;
    location_command_map["allow_methods"] = &ConfigParser::parse_allow_methods;
    location_command_map["autoindex"]     = &ConfigParser::parse_autoindex;
    location_command_map["return"]        = &ConfigParser::parse_return;
    location_command_map["upload_enable"] = &ConfigParser::parse_upload_enable;
    location_command_map["upload_store"]  = &ConfigParser::parse_upload_store;
    location_command_map["cgi_path"]      = &ConfigParser::parse_cgi_path;
    location_command_map["cgi_ext"]       = &ConfigParser::parse_cgi_ext;
}

Config ConfigParser::parse_config_file(const std::string& filename)
{
    Config config;

    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Config Error: The file could not be opened or found: " + filename);
    }

    std::vector<std::string> tokens = tokenize(file);

    if (tokens.empty()) {
        throw std::runtime_error("Config Error: File is empty.");
    }

    ServerConfig current_server;
    int in_server = 0;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        std::string token = tokens[i];

        if (in_server == 0) {
            if (token == "server") {
                in_server = 1;
                if (i + 1 < tokens.size() && tokens[i + 1] == "{") {
                    i++;
                } else {
                    throw std::runtime_error("Config Error: Expected '{' after 'server'.");
                }
            } else {
                throw std::runtime_error("Config Error: Unexpected token '" + token + "' outside of server block.");
            }
            continue;
        }

        if (in_server == 1) {
            if (token == "}") {
                config.servers.push_back(current_server);
                current_server = ServerConfig();
                in_server = 0;
                continue;
            }

            if (command_map.find(token) != command_map.end()) {
                (this->*command_map[token])(tokens, i, current_server);
            }
            else {
                throw std::runtime_error("Config Error: Unknown or invalid directive '" + token + "'.");
            }
        }
    }

    if (in_server != 0) {
        throw std::runtime_error("Config Error: Unexpected end of file (missing '}').");
    }

    if (config.servers.empty())
         throw std::runtime_error("Config Error: No valid server configuration found.");

    return config;
}

void ConfigParser::parse_listen(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    i++;
    std::string value = tokens[i];

    Listen new_listen;
    size_t colon = value.find(':');
    int dot = 0;
    if (colon != std::string::npos) { 
        int last = 0;
        const std::string &ip = value.substr(0, colon + 1);
        for (size_t j = 0; j < ip.size(); j++)
        {
            if (ip[j] == '.' || ip[j] == ':')
            {
                if ((j - last) > 3)
                    throw std::runtime_error("Config Error: 'parse_listen' 0");  
                int ip_octet = utils::atoi(value.substr(last, j - last));
                if (ip_octet == -1 || ip_octet > 255 || ip_octet < 0)
                    throw std::runtime_error("Config Error: 'parse_listen' 1");                 
                last = j + 1;
                if(ip[j] == '.')
                  dot++;
            }
        }
        if(dot != 3)
          throw std::runtime_error("Config Error: 'parse_listen' invalid ip format");
        int port = utils::atoi(value.substr(colon + 1));//kendi atoimiz yazılacak harf görürse hata verecek
        if (port == -1 || (port > 65535 || port <= 0))
        {
            throw std::runtime_error("Config Error: 'parse_listen' 2 ");
        }
        
        new_listen.ip = ip.substr(0,ip.size()-1);
        new_listen.port = port;
    } else { //burda hata vermesi gerekniyor mu? valuede belki askdfjalskdfja yazıyor port yapmaya mı uğraşıcaz onu
        new_listen.ip = "127.0.0.1";
        

        int port = utils::atoi(value);//kendi atoimiz yazılacak harf görürse hata verecek
        if (port == -1 || (port > 65535 || port <= 0))
            throw std::runtime_error("Config Error: 'parse_listen' 3");
        new_listen.port = port;
    }
    for (size_t j = 0; j < server.listens.size(); j++) {
        if (server.listens[j].ip == new_listen.ip && server.listens[j].port == new_listen.port) {
            throw std::runtime_error("Config Error: Duplicate listen " + value);
        }
    }

    server.listens.push_back(new_listen);

    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'listen' must end with ';'");
}

void ConfigParser::parse_server_name(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    if (!server.server_name.empty()) throw std::runtime_error("Config Error: Duplicate 'server_name'");
    
    i++;
    while (i < tokens.size() && tokens[i] != ";") {
        if (tokens[i] == "{" || tokens[i] == "}")
            throw std::runtime_error("Config Error: 'server_name' komutunda ';' unutulmus olabilir.");
        
        for (size_t j = 0; j < server.server_name.size(); j++) {
            if (server.server_name[j] == tokens[i])
                throw std::runtime_error("Config Error: Duplicate server name '" + tokens[i] + "'");
        }

        server.server_name.push_back(tokens[i]);
        i++;
    }
    
    if (server.server_name.empty())
        throw std::runtime_error("Config Error: Directive 'server_name' requires at least one argument.");

    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'server_name' must end with ';'");
}

void ConfigParser::parse_root(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    if (!server.root.empty()) throw std::runtime_error("Config Error: Duplicate 'root'");
    i++;
    server.root = tokens[i];

    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'root' must end with ';'");
}

void ConfigParser::parse_error_page(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    i++;
    std::vector<int> codes;
    // Hata kodlarını oku (örneğin: 500 502 503)
    while (i < tokens.size() && tokens[i] != ";" && isdigit(tokens[i][0])) {
        int code = utils::atoi(tokens[i]);
        if(code == -1)
            throw std::runtime_error("Config Error: 'error_page' 1");
        codes.push_back(code);
        i++;
    }

    std::string page = tokens[i]; // Son argüman hedef sayfadır (/errors/50x.html)

    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'error_page' must end with ';'");

    for (size_t j = 0; j < codes.size(); j++) {
        server.error_pages[codes[j]] = page;
    }
}

void ConfigParser::parse_client_max_body_size(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    i++;
    std::string val = tokens[i];
    size_t multiplier = 1;
    char last_char = val[val.length() - 1];

    if (last_char == 'M' || last_char == 'm') {
        multiplier = 1024 * 1024;
        val = val.substr(0, val.length() - 1);
    } else if (last_char == 'K' || last_char == 'k') {
        multiplier = 1024;
        val = val.substr(0, val.length() - 1);
    } else if (last_char == 'G' || last_char == 'g') {
        multiplier = 1024 * 1024 * 1024;
        val = val.substr(0, val.length() - 1);
    }

    int size = utils::atoi(val);
    if (size <= 0)
        throw std::runtime_error("Config Error: 'client_max_body_size' must be positive");
    server.client_max_body_size = size * multiplier;
    i++;
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'client_max_body_size' must end with ';'");
}

void ConfigParser::parse_index(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
    i++;
    if (!server.index.empty())
        throw std::runtime_error("Config Error: Duplicate 'index'");
    while (i < tokens.size() && tokens[i] != ";") {
        if (tokens[i] == "{" || tokens[i] == "}")
            throw std::runtime_error("Config Error: 'index' komutunda ';' unutulmus olabilir.");
        
        for (size_t j = 0; j < server.index.size(); j++) {
            if (server.index[j] == tokens[i])
                throw std::runtime_error("Config Error: Duplicate index '" + tokens[i] + "'");
        }
        
        server.index.push_back(tokens[i]);
        i++;
    }
    if (i >= tokens.size() || tokens[i] != ";") throw std::runtime_error("Config Error: 'index' must end with ';'");
}



std::vector<std::string> ConfigParser::tokenize(std::ifstream& file)
{
    std::vector<std::string> tokens;
    std::string current_token = "";
    char c;

    while (file.get(c)) {

        if (c == '#') {
            while (c != '\n' && file.get(c)) {
            }
            continue;
        }

        if (std::isspace(c))
        {
            if (!current_token.empty())
            {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }

        else if (c == '{' || c == '}' || c == ';')
        {
            if (!current_token.empty())
            {
                tokens.push_back(current_token);
                current_token.clear();
            }

            std::string special_char(1, c);
            tokens.push_back(special_char);
        }
        else
            current_token += c;
    }

    if (!current_token.empty())
        tokens.push_back(current_token);

    return tokens;
}
