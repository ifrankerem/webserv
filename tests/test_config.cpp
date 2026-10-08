#include "../includes/ConfigParser.hpp"
#include <iostream>

int main() {
    std::cout << "--- Testing ConfigParser ---" << std::endl;
    int errors = 0;
    
    // Valid Config Tests
    const char* valid_configs[] = {
        "tests/test_configs/valid.conf",
        "tests/test_configs/valid_body_sizes.conf",
        "tests/test_configs/valid_all_directives.conf"
    };
    int num_valid_tests = sizeof(valid_configs) / sizeof(valid_configs[0]);

    for (int i = 0; i < num_valid_tests; ++i) {
        try {
            ConfigParser parser;
            Config config = parser.parse_config_file(valid_configs[i]);
            if (config.servers.empty()) {
                std::cerr << "FAIL: " << valid_configs[i] << " sifir sunucu dondurdu." << std::endl;
                errors++;
            } else {
                //std::cout << "PASS: " << valid_configs[i] << " basariyla islendi." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "FAIL: " << valid_configs[i] << " hata firlatti: " << e.what() << std::endl;
            errors++;
        }
    }

    // Test 2: Multiple Args (String Concatenation) Testi
    try {
        ConfigParser parser;
        Config config = parser.parse_config_file("tests/test_configs/valid_multiple_args.conf");
        if (config.servers.empty() || 
            config.servers[0].server_name != "domain1.com domain2.com domain3.com" ||
            config.servers[0].index != "index.html index.php index.js" ||
            config.servers[0].locations[0].index != "main.html main.php") {
            std::cerr << "FAIL: Multiple args testi stringleri dogru birlestiremedi." << std::endl;
            errors++;
        } else {
            std::cout << "PASS: Multiple args (server_name ve index) basariyla boslukla birlestirildi." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Multiple args testi hata firlatti: " << e.what() << std::endl;
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
        "tests/test_configs/invalid_cases/invalid_empty.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_too_many_dots.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_letters.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_trailing_dot.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_only_dots.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_octet_overflow1.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_octet_overflow2.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_octet_overflow3.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_octet_overflow4.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_negative.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_missing_part.conf",
        "tests/test_configs/invalid_cases/ip/invalid_ip_hex.conf",
        "tests/test_configs/invalid_cases/port/invalid_port_negative.conf",
        "tests/test_configs/invalid_cases/port/invalid_port_letters.conf",
        "tests/test_configs/invalid_cases/port/invalid_port_missing.conf",
        "tests/test_configs/invalid_cases/port/invalid_port_multiple_colons.conf",
        "tests/test_configs/invalid_cases/body/invalid_body_size_mb.conf",
        "tests/test_configs/invalid_cases/body/invalid_body_size_negative.conf",
        "tests/test_configs/invalid_cases/body/invalid_body_size_only_letter.conf",
        "tests/test_configs/invalid_cases/body/invalid_body_size_middle_letter.conf",
        "tests/test_configs/invalid_cases/directives/invalid_server_name_duplicate.conf",
        "tests/test_configs/invalid_cases/directives/invalid_server_name_same_args.conf",
        "tests/test_configs/invalid_cases/directives/invalid_root_duplicate.conf",
        "tests/test_configs/invalid_cases/directives/invalid_autoindex_value.conf",
        "tests/test_configs/invalid_cases/directives/invalid_upload_enable_value.conf",
        "tests/test_configs/invalid_cases/directives/invalid_allow_methods_value.conf",
        "tests/test_configs/invalid_cases/directives/invalid_allow_methods_duplicate.conf",
        "tests/test_configs/invalid_cases/directives/invalid_return_code.conf",
        "tests/test_configs/invalid_cases/directives/invalid_return_no_path.conf",
        "tests/test_configs/invalid_cases/directives/invalid_return_code_letters.conf"
    };

    int num_invalid_tests = sizeof(invalid_configs) / sizeof(invalid_configs[0]);

    for (int i = 0; i < num_invalid_tests; ++i) {
        try {
            ConfigParser parser;
            Config config = parser.parse_config_file(invalid_configs[i]);
            std::cerr << "FAIL: " << invalid_configs[i] << " islenirken hata firlatilmadi!" << std::endl;
            errors++;
        } catch (const std::exception& e) {
            //std::cout << "PASS: " << invalid_configs[i] << " beklenen hatayi firlatti: " << e.what() << std::endl;
        }
    }

    if (errors > 0) {
        std::cerr << errors << " test(s) failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Tüm ConfigParser testleri basariyla gecti." << std::endl;
    return 0;
}