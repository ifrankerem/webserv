#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include "../http/HttpParser.hpp"

class RequestHandler;

enum ClientState 
{
    READING,
    PROCESSING,
    WRITING
};

class Client 
{
    private:
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
        ~Client();

        const std::string&  getServerIp() const;
        RequestHandler*     getHandler() const;
        ClientState         getState() const;
        HttpParser&         getParserObj();
        ParseState          getParseState() const;
        time_t              getLastActivity() const;
        int                 getServerPort() const;
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