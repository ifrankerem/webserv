#include "includes/core/ConfigParser.hpp"
#include "includes/event/ServerManager.hpp"

int main(int argc, char** argv) 
{
    try 
    {
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
    catch (const std::exception& e) 
    {
        std::cerr  << e.what() << "\n";
        return 1;
    }
    return 0;
}