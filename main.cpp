#include "includes/ConfigParser.hpp"

int main() {
    ConfigParser Config;

    Config.parse_config_file("conf/default.conf");
}