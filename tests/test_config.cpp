#include "../includes/ConfigParser.hpp"
#include <iostream>

int main() {
    std::cout << "--- Testing ConfigParser ---" << std::endl;
    int errors = 0;
    
    // Test 1: Gecerli Config
    try {
        ConfigParser parser;
        Config config = parser.parse_config_file("tests/test_configs/valid.conf");
        if (config.servers.empty()) {
            std::cerr << "FAIL: Gecerli config sifir sunucu dondurdu." << std::endl;
            errors++;
        } else {
            std::cout << "PASS: Gecerli config basariyla islendi." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Gecerli config hata firlatti: " << e.what() << std::endl;
        errors++;
    }

    // Invalid Config Tests
    const char* invalid_configs[] = {
        "tests/test_configs/invalid_cases/invalid.conf",
        "tests/test_configs/invalid_cases/invalid_missing_bracket.conf",
        "tests/test_configs/invalid_cases/invalid_missing_semicolon.conf",
        "tests/test_configs/invalid_cases/invalid_wrong_param.conf",
        "tests/test_configs/invalid_cases/invalid_port_overflow.conf",
        "tests/test_configs/invalid_cases/invalid_body_size_overflow.conf",
        "tests/test_configs/invalid_cases/invalid_duplicate_listen.conf",
        "tests/test_configs/invalid_cases/invalid_unknown_directive.conf",
        "tests/test_configs/invalid_cases/invalid_empty.conf"
    };

    int num_invalid_tests = sizeof(invalid_configs) / sizeof(invalid_configs[0]);

    for (int i = 0; i < num_invalid_tests; ++i) {
        try {
            ConfigParser parser;
            Config config = parser.parse_config_file(invalid_configs[i]);
            std::cerr << "FAIL: " << invalid_configs[i] << " islenirken hata firlatilmadi!" << std::endl;
            errors++;
        } catch (const std::exception& e) {
            std::cout << "PASS: " << invalid_configs[i] << " beklenen hatayi firlatti: " << e.what() << std::endl;
        }
    }

    if (errors > 0) {
        std::cerr << errors << " test(s) failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Tüm ConfigParser testleri basariyla gecti." << std::endl;
    return 0;
}