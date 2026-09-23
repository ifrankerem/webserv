#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() 
{
    command_map["listen"]               = &ConfigParser::parse_listen;
    command_map["server_name"]          = &ConfigParser::parse_server_name;
    command_map["root"]                 = &ConfigParser::parse_root;
    command_map["error_page"]           = &ConfigParser::parse_error_page;
    command_map["client_max_body_size"] = &ConfigParser::parse_client_max_body_size;
    command_map["index"]                = &ConfigParser::parse_index;
    command_map["location"]             = &ConfigParser::parse_location;
}

Config ConfigParser::parse_config_file(const std::string& filename)
{
    Config config;

    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Config Error: The file could not be opened or found: " + filename);
    }
    
    std::vector<std::string> tokens = tokenize(file);
    
    return config; 
}

void ConfigParser::parse_listen(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_server_name(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_root(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_error_page(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_client_max_body_size(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_index(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}

void ConfigParser::parse_location(std::vector<std::string>& tokens, size_t& i, ServerConfig& server)
{
}


void ConfigParser::parse_location_root(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_location_index(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_allow_methods(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_autoindex(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_return(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_upload_enable(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_upload_store(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_cgi_path(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
}

void ConfigParser::parse_cgi_ext(std::vector<std::string>& tokens, size_t& i, LocationConfig& location)
{
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