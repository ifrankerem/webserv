    #ifndef CLIENT_HPP
    #define CLIENT_HPP

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include "HttpParser.hpp"

    enum ClientState {
        READING,
        PROCESSING,
        WRITING,
        CLOSING
    };

    class Client {
    private:
        int                 _fd;
        struct sockaddr_in  _address;
        ClientState         _state;

        HttpParser          _parser;   

    public:
        Client();
        Client(int fd, struct sockaddr_in &address);
        ~Client();

        int getFd() const;
        ClientState getState() const;
        void setState(ClientState state);
        void parseData();                          
        ParseState getParseState() const;
        void feedData(const char* chunk, int size);
        HttpParser& getParserObj();
        void resetForNextRequest();
    };

    #endif