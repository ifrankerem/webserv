#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>

class Parser
{
    private:
        long listen;
        std::string server_name;
        int cmb_size;
    public:
        Parser();
        ~Parser();
        Parser(const Parser& other);
        Parser& operator=(const Parser& other);
};

#endif