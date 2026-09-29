#include "ResponseHandler.hpp"
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "Mime.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <vector>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
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


HttpResponse ResponseHandler::handle(const std::string& diskPath,const std::string& path,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	if(req.method == "GET")
		return handle_get(diskPath,req,loc,srv);
	else if(req.method == "POST")
		return handle_post(diskPath, path, req, loc,srv);
	else if(req.method == "DELETE")
		return handle_delete(diskPath, loc,srv);

	//bilinmeyen metot (PUT, PATCH...) -> 501
	HttpResponse response;
	response.status = 501;
	return response;
}


HttpResponse ResponseHandler::handle_get(const std::string& diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
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


HttpResponse ResponseHandler::serveFile(const std::string& diskPath,const ServerConfig& srv)
{
	HttpResponse x;
	std::ifstream file(diskPath.c_str());
	if (!file.is_open())
		return RequestHandler::error(403,&srv);

	std::istreambuf_iterator<char> first(file);
	std::istreambuf_iterator<char> last;
	x.body.assign(first, last);

	x.status = 200;
	x.headers["Content-Type"] = mimeType(diskPath);
	return x;
}

HttpResponse ResponseHandler::listDirectory(const std::string& diskPath,const ServerConfig& srv) //case for autoindex creating an html for indexing
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

HttpResponse ResponseHandler::handle_post(const std::string& diskPath,const std::string& path,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv) // A needs to give me bodylimit free body!!!
{
	HttpResponse x;
	if(!loc.upload_enable)
		return RequestHandler::error(403, &srv);
	if(loc.upload_store.empty())
		return RequestHandler::error(405, &srv); //ngnix returned 405 so i did it too

	std::map<std::string, std::string>::const_iterator it = req.headers.find("content-type");
	std::string type;
	if(it != req.headers.end())
		type = it->second;
	for(size_t i = 0; i < type.size(); ++i)
		type[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(type[i])));
	if(type.find("multipart/") != std::string::npos)
		return RequestHandler::error(415, &srv);

	//diskPath hedef dosya; handle() onu upload_store'dan uretti.
	//path ise temiz URL: location'in kendisiyse ya da '/' ile bitiyorsa
	//ortada dosya adi yok, yazacak bir sey de yok.
	if(path.size() <= loc.path.size() + 1 || path[path.size() - 1] == '/')
		return RequestHandler::error(400, &srv);

	std::ofstream outfile(diskPath.c_str(), std::ios::binary);
	if(!outfile)
		return RequestHandler::error(500, &srv);
	outfile << req.body;
	outfile.close();
	if(!outfile)
		return RequestHandler::error(500, &srv);

	x.status = 201;
	x.headers["Location"] = path;
	return x;
}


HttpResponse ResponseHandler::handle_delete(const std::string& diskPath, const LocationConfig &loc, const ServerConfig &srv)
{
	HttpResponse x;
	if(loc.upload_store.empty())
		return RequestHandler::error(403, &srv);
	struct stat info;
	if (::stat(diskPath.c_str(), &info) == -1) //stat() was not successful. The errno global variable is set to indicate the error.
	{
		if (errno == ENOENT || errno == ENOTDIR) // No such path or directory.
			return RequestHandler::error(404,&srv);
		else if(errno == EACCES || errno == EPERM) // Permission denied.
			return RequestHandler::error(403,&srv);
		else
			return RequestHandler::error(500,&srv); //  Internal Server Error:
	}
	if (S_ISDIR(info.st_mode) != 0)
		return RequestHandler::error(403, &srv);
	if (S_ISREG(info.st_mode) == 0)
		return RequestHandler::error(403, &srv);
	if (std::remove(diskPath.c_str()) != 0)
	{
		if(errno == ENOENT || errno == ENOTDIR)
			return RequestHandler::error(404, &srv);
		else if(errno == EACCES || errno == EPERM)
			return RequestHandler::error(403, &srv);
		else
			return RequestHandler::error(500, &srv);
	}

	x.status = 204;


	return x;
}
