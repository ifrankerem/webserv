#include "../includes/ServerManager.hpp"
#include "../includes/ConfigParser.hpp"
#include <iostream>

int main() {
    std::cout << "--- Testing Setup Server ---" << std::endl;
    
    try {
        ConfigParser parser;
        Config config = parser.parse_config_file("tests/test_configs/valid.conf");
        ServerManager manager(config);
        
        // Sadece socket bind işlemlerini yapıyoruz, run() cagrilmiyor!
        manager.setupServers();
        
        //std::cout << "PASS: Sunucu soketleri basariyla olusturuldu (bind edildi)." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: ServerManager hata firlatti: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}