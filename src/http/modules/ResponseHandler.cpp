#include "../../../includes/http/modules/ResponseHandler.hpp"
#include "../../../includes/core/Config.hpp"
#include "../../../includes/http/HttpRequest.hpp"
#include "../../../includes/http/HttpResponse.hpp"
#include "../../../includes/http/RequestHandler.hpp"
#include "../../../includes/core/Mime.hpp"
#include "../../../includes/core/Utils.hpp"
#include "../../../includes/http/modules/MultipartParser.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <unistd.h>
#include <vector>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sys/stat.h>

static std::string htmlEscape(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++)
	{
		switch (s[i])
		{
			case '&': out += "&amp;"; break;
			case '<': out += "&lt;"; break;
			case '>': out += "&gt;"; break;
			case '"': out += "&quot;"; break;
			case '\'': out += "&#39;"; break;
			default: out += s[i]; break;
		}
	}
	return out;
}

ResponseHandler::ResponseHandler()
{



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
	// HEAD de GET gibi hazirlanir; govdeyi serialize() atar ki Content-Length dogru kalsin
	if(req.method == "GET" || req.method == "HEAD")
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


static int check_stat(struct stat &info, const std::string& diskPath)
{
	if (::stat(diskPath.c_str(), &info) == -1)
	{
		// errno kullanmak yasak; access() ile varlik/yetki ayirt et
		if (::access(diskPath.c_str(), F_OK) == -1)
			return 404;
		if (::access(diskPath.c_str(), R_OK) == -1)
			return 403;
		return 500;
	}
	return 1;
}

HttpResponse ResponseHandler::handle_get(const std::string& diskPath,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	struct stat info;
	int stat_info = check_stat(info, diskPath);
	if(stat_info != 1)
		return RequestHandler::error(stat_info,&srv);

	if(S_ISDIR(info.st_mode) != 0)
	{
		if (!utils::endsWithSlash(req.path))
		{	//BAKILACAK
			std::string red_path;
			if(!req.query.empty())
				red_path = req.path + '/' + '?' +req.query;
			else
				red_path = req.path + '/';
			return ResponseHandler::redirect_handler(301, red_path);
		}
		std::vector<std::string> index_list = loc.index;
		if (index_list.empty())
			index_list = srv.index;
		for (size_t i = 0; i < index_list.size(); i++)
		{
			std::string temp_path = utils::joinPath(diskPath, index_list[i]);
			if (::stat(temp_path.c_str(), &info) == 0)
				return serveFile(temp_path,srv,info);
		}
		if(loc.autoindex)
			return listDirectory(diskPath, loc, srv);
		else
			return RequestHandler::error(404,&srv);
	}
	else {
		if (utils::endsWithSlash(req.path))
			return RequestHandler::error(404, &srv);
	}
	return serveFile(diskPath,srv,info); //Its normal file on the directory
}


HttpResponse ResponseHandler::serveFile(const std::string& diskPath,const ServerConfig& srv,const struct stat &info)
{
	HttpResponse x;
	if (!S_ISREG(info.st_mode))
		return RequestHandler::error(403, &srv);
	
	const size_t MAX_FILE_SIZE = 10 * 1024 * 1024; //max file site is 10MB
	if (static_cast<size_t>(info.st_size) > MAX_FILE_SIZE)
		return RequestHandler::error(500, &srv);

	std::ifstream file(diskPath.c_str(), std::ios::binary);
	if (!file.is_open())
		return RequestHandler::error(403, &srv);

	std::istreambuf_iterator<char> first(file);
	std::istreambuf_iterator<char> last;
	x.body.assign(first, last);

	x.status = 200;
	x.headers["Content-Type"] = mimeType(diskPath);
	return x;
}


//BAKILACAK
HttpResponse ResponseHandler::listDirectory(const std::string& diskPath, const LocationConfig& loc, const ServerConfig& srv) //case for autoindex creating an html for indexing
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

	std::string base;
	if (!loc.alias.empty())
		base = loc.alias;
	else if (!loc.root.empty())
		base = loc.root;
	else
		base = srv.root;

	std::string f_path = diskPath.substr(base.size(), diskPath.size()) + '/';

	std::string body = "<html>\n<head><title>Index of " + f_path +
		"</title></head>\n<body>\n<h1>Index of " + f_path + "</h1>\n<hr>\n<pre>\n";
	for (size_t i = 0; i < names.size(); i++)
	{
		std::string safe = htmlEscape(names[i]);
		body += "<a href=\"" + safe + "\">" + safe + "</a>\n";
	}
	body += "</pre>\n<hr>\n</body>\n</html>\n";

	HttpResponse x;
	x.status = 200;
	x.headers["Content-Type"] = "text/html; charset=UTF-8";
	x.body = body;
	return x;
}


//UPLOAD STORE GEREKLI MI

HttpResponse ResponseHandler::handle_post(const std::string& diskPath,const std::string& path,const HttpRequest &req, const LocationConfig& loc,const ServerConfig& srv)
{
	if(!loc.upload_enable)
		return RequestHandler::error(403, &srv);
	if(loc.upload_store.empty())
		return RequestHandler::error(405, &srv); //ngnix returned 405 so i did it too
	
	HttpResponse x;
	
	std::string type = utils::toLower(utils::getHeader(req.headers, "content-type"));
	if(type.find("multipart/") != std::string::npos)
	{
		std::vector<MultipartPart> parts;
		if(!parseMultipart(req.body, utils::getHeader(req.headers, "content-type"), parts))
			return RequestHandler::error(400, &srv);

		bool wrote = false;
		for(size_t i = 0; i < parts.size(); i++)
		{
			std::string filename = parts[i].filename;
			if(filename.empty())
				continue;		//skip the empty one its not a file
			if(filename.find('/') != std::string::npos	//secure for path traversal again!
				|| filename.find('\\') != std::string::npos
				|| filename.find("..") != std::string::npos
				|| filename.find('\0') != std::string::npos)
				return RequestHandler::error(400, &srv);

			std::ofstream outfile(utils::joinPath(loc.upload_store, filename).c_str(), std::ios::binary);
			if(!outfile)
				return RequestHandler::error(500, &srv);
			outfile.write(parts[i].content.data(), parts[i].content.size());
			outfile.close();
			if(!outfile)
				return RequestHandler::error(500, &srv);
			wrote = true;
		}
		if(!wrote)
			return RequestHandler::error(400, &srv);

		x.status = 201;
		x.headers["Location"] = path;
		return x;
	}

	std::string target = diskPath;
	std::string url = path;
	struct stat st;
	bool isDir = ::stat(diskPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
	if(path == loc.path || utils::endsWithSlash(req.path) || isDir)
	{
		if(!isDir)
			return RequestHandler::error(404, &srv);
		static size_t counter = 0;
		std::string name;
		do
			name = "upload_" + utils::toString(counter++);
		while(::access(utils::joinPath(diskPath, name).c_str(), F_OK) == 0);
		target = utils::joinPath(diskPath, name);
		url = utils::joinPath(path, name);
	}

	std::ofstream outfile(target.c_str(), std::ios::binary);
	if(!outfile)
		return RequestHandler::error(500, &srv);
	outfile.write(req.body.data(), req.body.size());
	outfile.close();
	if(!outfile)
		return RequestHandler::error(500, &srv);

	x.status = 201;
	x.headers["Location"] = url;
	return x;
}


HttpResponse ResponseHandler::handle_delete(const std::string& diskPath, const LocationConfig &loc, const ServerConfig &srv)
{
	HttpResponse x;
	if(loc.upload_store.empty())
		return RequestHandler::error(403, &srv);
	struct stat info;
	int stat_info = check_stat(info, diskPath);
	if(stat_info != 1)
		return RequestHandler::error(stat_info,&srv);
	if (S_ISREG(info.st_mode) == 0)
		return RequestHandler::error(403, &srv);
	if (std::remove(diskPath.c_str()) != 0)
		return RequestHandler::error(403, &srv);

	x.status = 204;

	return x;
}


void ResponseHandler::handle_cgi_child(const int output_fd,const int input_fd,std::map<std::string, std::string>& env_variables,const std::string& scriptPath,const std::string& interpreter)
{
	// Script needs to read request body from STDIN, so body comes from parent through input pipe
	dup2(input_fd, STDIN_FILENO);
	close(input_fd); // there is no need for the twin, stdin now points to the pipe

	// Scripts will use STDOUT, so redirect stdout to output pipe to give response back to parent
	dup2(output_fd, STDOUT_FILENO);
	close(output_fd); // stdout now points to the pipe

	for (int i = 3; i < 1024; ++i) {
        close(i);
    }
	std::vector<char*> envp;
	std::vector<std::string> temp_env = utils::mapToEnv(env_variables);
	for(size_t i = 0; i < temp_env.size() ; i++)
		envp.push_back(const_cast<char*>(temp_env[i].c_str()));
	envp.push_back(NULL);

	std::string scriptName = scriptPath;
	std::string scriptDir;
	size_t slash = scriptPath.rfind('/');
	if (slash != std::string::npos)
	{
		scriptDir = scriptPath.substr(0, slash);
		scriptName = scriptPath.substr(slash + 1);
		chdir(scriptDir.c_str());
	}

	std::string interp = interpreter;
	//if there is relative interpreter path
	if (!interp.empty() && interp[0] != '/' && !scriptDir.empty() && scriptDir[0] != '/')
	{
		size_t components = 0;
		size_t i = 0;
		while (i < scriptDir.size())
		{
			while (i < scriptDir.size() && scriptDir[i] == '/')
				i++;
			if (i >= scriptDir.size())
				break;
			components++;
			while (i < scriptDir.size() && scriptDir[i] != '/')
				i++;
		}
		std::string prefix;
		for (size_t j = 0; j < components; j++)
			prefix += "../";
		interp = prefix + interp;
	}

	char* argv[3];
	argv[0] = const_cast<char*>(interp.c_str());
	argv[1] = const_cast<char*>(scriptName.c_str());
	argv[2] = NULL;

	execve(interp.c_str(), argv, &envp[0]);

	// if execve fails these lines of code will work

	std::exit(1);
}
