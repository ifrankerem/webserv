#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "Config.hpp"
#include <fstream>

class ConfigParser 
{
    private:
        typedef void (ConfigParser::*ParseFunction)(std::vector<std::string>&, size_t&, ServerConfig&);
        typedef void (ConfigParser::*LocationParseFunction)(std::vector<std::string>&, size_t&, LocationConfig&);
        
        std::map<std::string, ParseFunction>            command_map;
        std::map<std::string, LocationParseFunction>    location_command_map;

        void parse_listen(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_server_name(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_root(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_error_page(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_client_max_body_size(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_index(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        
        void parse_location(std::vector<std::string>& tokens, size_t& i, ServerConfig& server);
        void parse_location_root(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_location_alias(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_location_index(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_location_client_max_body_size(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_allow_methods(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_autoindex(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_return(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_upload_enable(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_upload_store(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);
        void parse_cgi(std::vector<std::string>& tokens, size_t& i, LocationConfig& location);

        std::vector<std::string> tokenize(std::ifstream& file);
    public:
        ConfigParser(); 
        
        Config parse_config_file(const std::string& filename);
};

#endif