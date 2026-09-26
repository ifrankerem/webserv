#include <iostream>
#include "includes/ConfigParser.hpp"
#include "includes/ServerManager.hpp"

// Terminal çıktısını renklendirmek (isteğe bağlı) ve düzenlemek için
void debug_config(const Config& config) {
    std::cout << "\n========================================\n";
    std::cout << "         CONFIG PARSER DEBUGGER         \n";
    std::cout << "========================================\n";

    if (config.servers.empty()) {
        std::cout << "Hata: Hic server bulunamadi!\n";
        return;
    }

    for (size_t i = 0; i < config.servers.size(); i++) {
        const ServerConfig& s = config.servers[i];
        std::cout << "\n[SERVER " << i + 1 << "]\n";
        std::cout << "  Server Name          : " << (s.server_name.empty() ? "(yok)" : s.server_name) << "\n";
        std::cout << "  Root                 : " << (s.root.empty() ? "(yok)" : s.root) << "\n";
        std::cout << "  Index                : " << (s.index.empty() ? "(yok)" : s.index) << "\n";
        std::cout << "  Client Max Body Size : " << s.client_max_body_size << " bytes\n";

        std::cout << "  Listens:\n";
        for (size_t j = 0; j < s.listens.size(); j++) {
            std::cout << "    - " << s.listens[j].ip << ":" << s.listens[j].port << "\n";
        }

        std::cout << "  Error Pages:\n";
        // C++98'de map'in içinde dönmek için const_iterator kullanmak zorundayız
        for (std::map<int, std::string>::const_iterator it = s.error_pages.begin(); it != s.error_pages.end(); ++it) {
            std::cout << "    - Code " << it->first << " -> " << it->second << "\n";
        }

        std::cout << "\n  --- LOCATIONS (" << s.locations.size() << ") ---\n";
        for (size_t j = 0; j < s.locations.size(); j++) {
            const LocationConfig& loc = s.locations[j];
            std::cout << "  [" << i + 1 << "." << j + 1 << "] Path: " << loc.path << "\n";
            std::cout << "      Root             : " << (loc.root.empty() ? "(yok)" : loc.root) << "\n";
            std::cout << "      Index            : " << (loc.index.empty() ? "(yok)" : loc.index) << "\n";
            std::cout << "      Autoindex        : " << (loc.autoindex ? "on" : "off") << "\n";
            std::cout << "      Upload Enable    : " << (loc.upload_enable ? "on" : "off") << "\n";
            std::cout << "      Upload Store     : " << (loc.upload_store.empty() ? "(yok)" : loc.upload_store) << "\n";
            
            if (loc.redirect_code != 0) {
                std::cout << "      Redirect         : " << loc.redirect_code << " -> " << loc.redirect_target << "\n";
            }

            std::cout << "      Allowed Methods  : ";
            for (size_t k = 0; k < loc.allowed_methods.size(); k++) {
                std::cout << loc.allowed_methods[k] << " ";
            }
            std::cout << "\n";

            if (!loc.cgi_pass.empty()) {
                std::cout << "      CGI Pass         :\n";
                for (std::map<std::string, std::string>::const_iterator it = loc.cgi_pass.begin(); it != loc.cgi_pass.end(); ++it) {
                    std::cout << "        Ext: '" << it->first << "' -> Path: '" << it->second << "'\n";
                }
            }
            std::cout << "\n";
        }
        std::cout << "----------------------------------------\n";
    }
}

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int main() {
    try {
        ConfigParser Conf;
        Config serv;
        serv = Conf.parse_config_file("conf/default.conf");
        ServerManager manage(serv);
        
        manage.setupServers();
        manage.run();
        //debug_config(serv);
        
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[!] KARDESIM PARSER PATLADI: " << e.what() << "\n\n";
    }

	struct stat info;
	std::string path = "/home/kero/Desktop/Projeler/webservortak/conf";
	if(::stat(path.c_str(), &info) != -1)
		std::cout << info.st_mode << info.st_gid << "    "  << info.st_uid << "      " << info.st_size << std::endl;
}