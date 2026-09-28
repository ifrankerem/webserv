#include "ResponseHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "Mime.hpp"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <vector>
#include <cstdio>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

ResponseHandler::ResponseHandler()
{



}




ResponseHandler::ResponseHandler(const ResponseHandler& other)
{
    *this = other;
}

ResponseHandler& ResponseHandler::operator=(const ResponseHandler& other)
{
    if (this != &other)
    {
        
    }

    return *this;
}

ResponseHandler::~ResponseHandler()
{
}

HttpResponse ResponseHandler::redirect_handler(int status_code,const std::string &target)
{
	HttpResponse response;
	response.status = status_code;
	response.headers["Location"] = target;
	return response;
}


HttpResponse ResponseHandler::handle(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	if(req.method == "GET")
		return handle_get(diskPath,req,loc,srv);
// 	else if(req.method == "POST")
// 		return handle_post(diskPath, req, loc,srv);
// 	else if(req.method == "DELETE")
// 	{
// 
// 	}

	//TODO B2/B3: fill the branches above; until then keep the build green
	(void)loc;
	(void)srv;
	HttpResponse response;
	response.status = 501;
	return response;
}


HttpResponse ResponseHandler::handle_get(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	struct stat info;
	if (::stat(diskPath.c_str(), &info) == -1) //stat() was not successful. The errno global variable is set to indicate the error.
	{
		if (errno == ENOENT) // No such path or directory.
			return RequestHandler::error(404,&srv);
		else if(errno == EACCES) // Permission denied.
			return RequestHandler::error(403,&srv);
		else 
			return RequestHandler::error(500,&srv); //  Internal Server Error:

		//TODO maybe more if and elses this part for more status codes
	}

	if(S_ISDIR(info.st_mode) != 0)
	{
		std::string indexPath = diskPath;
		if (req.path[req.path.size() - 1] != '/')
		{
			std::string red_path;
			if(!req.query.empty())
				red_path = req.path + '/' + '?' +req.query;
			else
				red_path = req.path + '/';
			return ResponseHandler::redirect_handler(301, red_path);
		}
		else if(indexPath[indexPath.size() - 1] != '/')
			indexPath += '/';
		indexPath += loc.index;
		if (::stat(indexPath.c_str(), &info) == 0)
			return serveFile(indexPath,srv);
		else if(loc.autoindex)
			return listDirectory(diskPath,srv);
		else
			return RequestHandler::error(403,&srv);
	}
	else {
		if (req.path[req.path.size() - 1] == '/')
			return RequestHandler::error(404, &srv);
	}
	return serveFile(diskPath,srv); //Its normal file on the directory
}


HttpResponse ResponseHandler::serveFile(const std::string diskPath,const ServerConfig& srv)
{
	HttpResponse x;
	std::ifstream file(diskPath.c_str());
	if (!file.is_open())
		return RequestHandler::error(403,&srv);

	char c;
	while(file.get(c))
	{
		x.body += c;
	}

	x.status = 200;
	x.headers["Content-Type"] = mimeType(diskPath);
	return x;
}

HttpResponse ResponseHandler::listDirectory(const std::string diskPath,const ServerConfig& srv) //case for autoindex creating an html for indexing
{
	DIR* dir = opendir(diskPath.c_str());
	if (dir == NULL)
		return RequestHandler::error(403,&srv);

	std::vector<std::string> names;
	struct dirent* entry = readdir(dir);
	while (entry != NULL)
	{
		std::string name = entry->d_name;
		if (name != ".")
		{
			struct stat info;
			std::string full = diskPath + "/" + name;
			if (::stat(full.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
				name += "/";
			names.push_back(name);
		}
		entry = readdir(dir);
	}
	closedir(dir);

	std::sort(names.begin(), names.end());
	
	std::string f_path = diskPath.substr(srv.root.size(),diskPath.size()) + '/';

	std::string body = "<html>\n<head><title>Index of " + f_path +
		"</title></head>\n<body>\n<h1>Index of " + f_path + "</h1>\n<hr>\n<pre>\n";
	for (size_t i = 0; i < names.size(); i++)
		body += "<a href=\"" + names[i] + "\">" + names[i] + "</a>\n";
	body += "</pre>\n<hr>\n</body>\n</html>\n";

	HttpResponse x;
	x.status = 200;
	x.headers["Content-Type"] = "text/html; charset=UTF-8";
	x.body = body;
	return x;
}

// HttpResponse ResponseHandler::handle_post(const std::string diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
// {
// 	
// }


