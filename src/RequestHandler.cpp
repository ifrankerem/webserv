#include "RequestHandler.hpp"
#include "Config.hpp"
#include "HttpResponse.hpp"
#include "ResponseHandler.hpp"
#include <cstddef>

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
		if (path.compare(0, loc.path.size(), loc.path) == 0)
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


size_t RequestHandler::bodyLimit(const HttpRequest &req)
{
	const LocationConfig *loc = findLocation(req.path);
	if(loc == NULL)
		return server->client_max_body_size;
	return loc->client_max_body_size;
}


HttpResponse RequestHandler::error(int status)
{
	HttpResponse response;
	response.status = status;
	//TODO there will be a document reading part
	if(status == 404)
	{
		response.body = "<html><body><h1>404 Not Found</h1></body></html>";
		response.headers["Content-Type"] = "text/html";
	}
	
	return response;
}



Step RequestHandler::handle(const HttpRequest &req)
{
	std::string path;
	if (!normalizePath(req.path, path))
	{
		Step x;
		x.kind = Step::RESPOND;
		x.response = error(400);
		return x;
	}

	const LocationConfig *best = findLocation(path);

	if(best == NULL)
	{
		//TODO conf has no location so maybe use Default Settings?
		struct Step x = Step();
		x.kind = Step::RESPOND;
		x.response = error(404);
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
	    x.response = error(405);
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
		std::string diskPath = resolveDiskPath(*best, path);
		struct Step x = Step();
		x.kind = Step::RESPOND;
		//TODO B2: serve diskPath (200 / 403 / 404); for now show what routing produced
		x.response.status = 200;
		x.response.headers["Content-Type"] = "text/plain";
		x.response.body = diskPath;
		return x;
	}
}