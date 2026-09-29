#include "RequestHandler.hpp"
#include "Config.hpp"
#include "HttpResponse.hpp"
#include "ResponseHandler.hpp"
#include <cstddef>
#include <sstream>
#include <cstddef>
#include <vector>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>


RequestHandler::RequestHandler(const ServerConfig& server) : server(&server)
{
}

RequestHandler::RequestHandler(const RequestHandler& other)
{
    *this = other;
}

RequestHandler& RequestHandler::operator=(const RequestHandler& other)
{
    if (this != &other)
    {

    }

    return *this;
}

RequestHandler::~RequestHandler()
{
}


const LocationConfig* RequestHandler::findLocation(const std::string& path)
{
	const LocationConfig* best = NULL;
	for(size_t i = 0 ; i < server->locations.size(); i++)
	{
		const LocationConfig &loc = server->locations[i];
		bool pathMatch = path.compare(0, loc.path.size(), loc.path) == 0;
		bool pathBoundary = loc.path == "/" || path.size() == loc.path.size()
			|| (!loc.path.empty() && loc.path[loc.path.size() - 1] == '/')
			|| (path.size() > loc.path.size() && path[loc.path.size()] == '/');
		if (pathMatch && pathBoundary)
		{
			if(best == NULL || loc.path.size() >= best->path.size())
				best = &loc;
		}
	}
	return best;
}

bool RequestHandler::normalizePath(const std::string& path, std::string& out)
{
	if (path.empty() || path[0] != '/')
		return false;
	if (path.find('\0') != std::string::npos)
		return false;

	std::vector<std::string> parts;
	size_t i = 0;
	while (i < path.size())
	{
		size_t j = path.find('/', i);
		if (j == std::string::npos)
			j = path.size();

		std::string part = path.substr(i, j - i);
		if (part == "..")
		{
			if (parts.empty())
				return false;
			parts.erase(parts.end() - 1);
		}
		else if (!part.empty() && part != ".")
			parts.push_back(part);
		i = j + 1;
	}

	out = "/";
	for (size_t k = 0; k < parts.size(); k++)
	{
		out += parts[k];
		if (k + 1 < parts.size())
			out += "/";
	}
	return true;
}

std::string RequestHandler::resolveDiskPath(const LocationConfig& loc, const std::string& path)
{
	std::string rest = path.substr(loc.path.size());
	std::string root = loc.root;

	if (root.empty())
		root = server->root;
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	if (rest.empty() || rest[0] != '/')
		rest = "/" + rest;

	return root + rest;
}

std::string RequestHandler::resolveUploadPath(const LocationConfig& loc, const std::string& path)
{
	std::string root = loc.upload_store;
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	std::string rest = path.substr(loc.path.size());
	if (rest.empty() || rest[0] != '/')
		rest = "/" + rest;
	return root + rest;
}


size_t RequestHandler::bodyLimit(const HttpRequest &req)
{
	const LocationConfig *loc = findLocation(req.path);
	if(loc == NULL)
		return server->client_max_body_size;
	return loc->client_max_body_size;
}

static HttpResponse defaultErrorPage(int status)
{
	std::ostringstream line;
	line << status;

	HttpResponse x;
	x.status = status;
	x.headers["Content-Type"] = "text/html; charset=UTF-8";
	x.body =
		"<html>\n<head><title>" + line.str() + "</title></head>\n"
		"<body>\n<center><h1>" + line.str() + "</h1></center>\n"
		"<hr><center>webserv</center>\n</body>\n</html>\n";
	return x;
}


HttpResponse RequestHandler::error(int status, const ServerConfig *server)
{
	HttpResponse x;

	std::map<int, std::string>::const_iterator it = server->error_pages.find(status);
	if (it == server->error_pages.end())
		return defaultErrorPage(status);

	std::string path = server->root + it->second;
	std::ifstream file(path.c_str());

	if (!file.is_open())
		return defaultErrorPage(status);

	std::istreambuf_iterator<char> first(file);
	std::istreambuf_iterator<char> last;
	x.body.assign(first, last);

	x.status = status;
	x.headers["Content-Type"] = "text/html; charset=UTF-8";
	return x;
}


Step RequestHandler::handle(const HttpRequest &req)
{
	std::string path;
	if (!normalizePath(req.path, path))
	{
		Step x;
		x.kind = Step::RESPOND;
		x.response = error(400,server);
		return x;
	}

	const LocationConfig *best = findLocation(path);

	if(best == NULL)
	{
		//TODO conf has no location so maybe use Default Settings?
		struct Step x = Step();
		x.kind = Step::RESPOND;
		x.response = error(404,server);
		return x;
	}

	bool allowed = false;

	for(size_t i = 0 ; i < best->allowed_methods.size(); i++)
	{
		if(best->allowed_methods[i].compare(req.method) == 0)
		{
			allowed = true;
			break;
		}
	}

	if (allowed == false)
	{
	    std::string allow;
	    for (size_t i = 0; i < best->allowed_methods.size(); i++)
	    {
	        if (i > 0)
	            allow += ", ";
	        allow += best->allowed_methods[i];
	    }

	    Step x;
	    x.kind = Step::RESPOND;
	    x.response = error(405,server);
	    x.response.headers["Allow"] = allow;
	    return x;
	}

	if(best->redirect_code != 0)
	{
		struct Step x = Step();
		x.kind = Step::RESPOND;
		x.response = ResponseHandler::redirect_handler(best->redirect_code,best->redirect_target);
		return x;
	}
	else {
		std::string diskPath;
		//POST ve DELETE ayni klasorle calisir: hedefi tek yerde hesapla
		if ((req.method == "DELETE" || req.method == "POST") && !best->upload_store.empty())
			diskPath = resolveUploadPath(*best, path);
		else
			diskPath = resolveDiskPath(*best, path); // its final version of path root + req
		struct Step x = Step();
		x.kind = Step::RESPOND;
		x.response = ResponseHandler::handle(diskPath, path, req, *best, *server);
		return x;
	}
}
