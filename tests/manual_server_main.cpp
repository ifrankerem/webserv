#include <iostream>
#include "../includes/ConfigParser.hpp"
#include "../includes/ServerManager.hpp"

// Elle test etmek icin ayri main.
// Kullanim: ./webserv_manual conf/manual_test.conf
// (varsayilan config argumani yoktur; argumansiz calistirilmamalidir)

int main(int argc, char** argv)
{
    try {
        if (argc < 2)
        {
            std::cerr << "Kullanim: " << argv[0] << " <config_dosyasi>\n";
            return 1;
        }

        ConfigParser Conf;
        Config serv;
        serv = Conf.parse_config_file(argv[1]);
        ServerManager manage(serv);

        manage.setupServers();
        manage.run();
    }
    catch (const std::exception& e) {
        std::cerr << "\n[!] Sunucu baslatilamadi: " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}
