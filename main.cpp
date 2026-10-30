#include <iostream>
#include "core/ConfigParser.hpp"
#include "event/ServerManager.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int main(int argc, char** argv) {
    try {
        if (argc > 2)
        {
            std::cerr << "Usage: ./webserv [file.conf]\n";
            return 1;
        }
        std::string filename = "conf/default.conf";
        if (argc != 1)
            filename = argv[1];

        ConfigParser Conf;
        Config serv;
        serv = Conf.parse_config_file(filename);
        ServerManager manage(serv);
        
        manage.setupServers();
        manage.run();        
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[!] KARDESIM PARSER PATLADI: " << e.what() << "\n\n";
    }
}