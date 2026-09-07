#ifndef PARSE_HPP
#define PARSE_HPP

#include <string>

class Parse
{
	private:

	public:

	enum	ParseResult
	{
		PARSE_OK,
		PARSE_ERROR,
		PARSE_INCOMPLETE
	};
	
 	static ParseResult parse(const std::string& buf, size_t& consumed);
  	int is_complete(); //TODO 3 bayt geriden gelerek surekli bitip bitmedigini kontrol edicek
   	

};

#endif