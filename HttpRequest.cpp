#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	this->flag = 0;
}

HttpRequest::HttpRequest(const HttpRequest& other)
{
    *this = other;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
    if (this != &other)
    {
        
    }

    return *this;
}

HttpRequest::~HttpRequest()
{
}

HttpRequest::State HttpRequest::parse(const std::string& buf)
{
		//TODO PARSING USE STATES 
}

bool HttpRequest::is_complete(const std::string& buf,ssize_t nbr)
{
	ssize_t index = nbr - 3;
	if(index < 0)
		index = 0;  //INFO out of bounds!
	if(this->flag == 1)
		return true;
	size_t pos = buf.find("\r\n\r\n", index);
	if (pos != std::string::npos)
		return true;
	return false;
}

void HttpRequest::setByteSent(ssize_t n)
{
	this->bytesSent = n;
}


void HttpRequest::incrementFlag()
{
	this->flag++;
}



