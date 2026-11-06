#ifndef CLIENT_HPP
#define CLIENT_HPP

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
        int                 _fd; // bu nerde kullanılıyor
        struct sockaddr_in  _address; // bu nerde kullanılıyo
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

        struct sockaddr_in  getAdress();//silinebilir
        const std::string&  getServerIp() const;
        RequestHandler*     getHandler() const;
        ClientState         getState() const;
        HttpParser&         getParserObj();
        ParseState          getParseState() const;
        time_t              getLastActivity() const;
        int                 getServerPort() const;
        int                 getFd() const; //silinebilir
        void                setState(ClientState state);
        void                setServerPort(int port);
        void                setServerIp(const std::string& ip);
        void                setHandler(RequestHandler* handler);

        const char*         sendPtr() const;
        size_t              sendSize() const;
        void                resetForNextRequest();
        void                feedData(const char* chunk, int size);
        void                parseData();                          
        void                prepareResponse(const std::string& serialized);
        bool                hasPendingData() const;
        void                advanceSent(size_t n);
        void                clearHandler();
        void                touch();
};

#endif