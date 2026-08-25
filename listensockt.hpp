#ifndef LISTENSOCKT_HPP
#define LISTENSOCKT_HPP

#include "sockt.hpp"

class listensockt : public sockt
{
	private:

	public:
		listensockt();
		listensockt(int domain,int type , int protocol);
		~listensockt();
		listensockt(const listensockt& other);
		listensockt& operator=(const listensockt& other);
};

#endif