#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>
#include "../http/HttpParser.hpp"

class RequestHandler;

enum ClientState 
{
    READING,
    PROCESSING,
    WRITING,
    CLOSING
};

class Client 
{
    private:
        int                 _fd;
        struct sockaddr_in  _address;
        ClientState         _state;

        HttpParser          _parser;

        std::string         _writeBuffer;
        size_t              _bytesSent;
        int                 _serverPort;
        std::string         _serverIp;
        RequestHandler*     _handler;
        time_t              _lastActivity;

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
        struct sockaddr_in        getAdress();

        void        setServerPort(int port);
        int         getServerPort() const;
        void        setServerIp(const std::string& ip);
        const std::string& getServerIp() const;
        void        prepareResponse(const std::string& serialized);
        bool        hasPendingData() const;
        const char* sendPtr() const;
        size_t      sendSize() const;
        void        advanceSent(size_t n);

        RequestHandler* getHandler() const;
        void            setHandler(RequestHandler* handler);
        void            clearHandler();

        void            touch();
        time_t          getLastActivity() const;
};

#endif