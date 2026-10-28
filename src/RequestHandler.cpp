#include "../includes/RequestHandler.hpp"
#include "../includes/Config.hpp"
#include "../includes/HttpResponse.hpp"
#include "../includes/ResponseHandler.hpp"
#include "../includes/Utils.hpp"
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>


RequestHandler::RequestHandler(const ServerConfig& server) : server(&server), cgiActive(false), cgiPid(-1), cgiPipeFd(-1), cgiStdinFd(-1), cgiInOffset(0)
{
}

RequestHandler::~RequestHandler()
{
	if (cgiActive)
	{
		if (cgiPipeFd != -1)
			close(cgiPipeFd);
		if (cgiStdinFd != -1)
			close(cgiStdinFd);
		if (cgiPid != -1)
		{
			// SIGKILL: yakalanamaz, yok sayilamaz. SIGTERM'i yok sayan bir script
			// olsaydi asagidaki bloklayan waitpid hic donmez, tum sunucu donardi.
			kill(cgiPid, SIGKILL);
			waitpid(cgiPid, NULL, 0);
		}
	}
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
	if (!loc.alias.empty())
	{
		std::string alias = loc.alias;
		if (!alias.empty() && alias[alias.size() - 1] == '/')
			alias.erase(alias.size() - 1);
		std::string rest = path.substr(loc.path.size());
		if (rest.empty() || rest[0] != '/')
			rest = "/" + rest;
		return alias + rest;
	}

	std::string root = loc.root;
	if (root.empty())
		root = server->root;
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	return root + path;
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

static std::map<std::string, std::string> buildCgiEnv(const HttpRequest& req,
	const LocationConfig& loc, const ServerConfig& srv, const std::string& scriptPath)
{
	std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = req.method;
	env["SCRIPT_NAME"] = req.path;
	env["SCRIPT_FILENAME"] = scriptPath;
	env["PATH_INFO"] = req.path;
	env["REQUEST_URI"] = req.path;
	env["PATH_TRANSLATED"] = "";
	env["QUERY_STRING"] = req.query;
	env["SERVER_PROTOCOL"] = req.version;

	env["CONTENT_TYPE"] = utils::getHeader(req.headers, "Content-Type");
	env["CONTENT_LENGTH"] = utils::toString(req.body.size());
	std::string host_name = utils::getHeader(req.headers, "Host");
	env["SERVER_NAME"] = srv.server_name[0]; //default is the first one
	for(size_t i = 0; i < srv.server_name.size();i++)
	{
		if(host_name == srv.server_name[i])
		{
			env["SERVER_NAME"] = host_name;
			break;
		}
	}
	
	env["SERVER_PORT"] = utils::toString(static_cast<size_t>(req.server_port));
	env["REMOTE_ADDR"] = req.remote_addr;
	env["REDIRECT_STATUS"] = "200";
	std::string userAgent = utils::getHeader(req.headers, "User-Agent");
	if (!userAgent.empty())
		env["HTTP_USER_AGENT"] = userAgent;

	std::string host = utils::getHeader(req.headers, "Host");
	if (!host.empty())
		env["HTTP_HOST"] = host;

	for (std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) 
    {
        std::string key = it->first;
        std::string env_key = "HTTP_";
        for (size_t i = 0; i < key.size(); i++) 
        {
            if (key[i] == '-')
                env_key += '_';
            else
                env_key += std::toupper(key[i]);
        }
        if (env.find(env_key) == env.end()) {
            env[env_key] = it->second;
        }
    }
	(void)loc;
	return env;
}

// raw tuketilir: govde kopyalanmadan response.body'ye swap edilir (100 MB cikti = 0 ek kopya)
static void parseCgiOutput(std::string& raw, HttpResponse& response)
{
	response.status = 200;

	size_t blank = raw.find("\r\n\r\n");
	size_t headerEndLen = 4;
	if (blank == std::string::npos)
	{
		blank = raw.find("\n\n");
		headerEndLen = 2;
	}

	std::string headersPart;
	if (blank != std::string::npos)
	{
		headersPart = raw.substr(0, blank);
		raw.erase(0, blank + headerEndLen);
	}
	response.body.swap(raw);

	std::string remaining = headersPart;
	while (!remaining.empty())
	{
		size_t lineEnd = remaining.find("\r\n");
		size_t skip = 2;
		if (lineEnd == std::string::npos)
		{
			lineEnd = remaining.find('\n');
			skip = 1;
		}

		std::string line;
		if (lineEnd == std::string::npos)
		{
			line = remaining;
			remaining.clear();
		}
		else
		{
			line = remaining.substr(0, lineEnd);
			remaining = remaining.substr(lineEnd + skip);
		}

		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			size_t first = value.find_first_not_of(" \t");
			if (first != std::string::npos)
				value = value.substr(first);

			if (key == "Status")
				response.status = std::atoi(value.c_str());
			else
				response.headers[key] = value;
		}
	}

	if (response.headers.find("Content-Type") == response.headers.end())
		response.headers["Content-Type"] = "text/html; charset=UTF-8";

}

size_t RequestHandler::bodyLimit(const HttpRequest &req)
{
	const LocationConfig *loc = findLocation(req.path);
	// location limit yazmadiysa (0) server limitini miras alir, nginx gibi
	if(loc == NULL || loc->client_max_body_size == 0)
		return server->client_max_body_size;
	return loc->client_max_body_size;
}

HttpResponse RequestHandler::error(int status, const ServerConfig *server)
{
	HttpResponse x;

	std::map<int, std::string>::const_iterator it = server->error_pages.find(status);
	if (it == server->error_pages.end())
		return utils::defaultErrorPage(status);

	// sayfa URI'si normal istek gibi cozulur: eslesen location'in root/alias'i
	RequestHandler tmp(*server);
	const LocationConfig* loc = tmp.findLocation(it->second);
	std::string path = loc ? tmp.resolveDiskPath(*loc, it->second) : server->root + it->second;
	std::ifstream file(path.c_str());

	if (!file.is_open())
		return utils::defaultErrorPage(status);

	std::istreambuf_iterator<char> first(file);
	std::istreambuf_iterator<char> last;
	//For performance i use iterators while loop is so slow for these interections
	// 
	x.body.assign(first, last);

	x.status = status;
	x.headers["Content-Type"] = "text/html; charset=UTF-8";
	return x;
}

void RequestHandler::closeCgiStdin(Step& x)
{
	if (cgiStdinFd != -1)
	{
		x.unwatch.push_back(cgiStdinFd);
		close(cgiStdinFd);
		cgiStdinFd = -1;
		std::string().swap(cgiBody); // CGI artik okumuyor: govdenin bellegini birak
	}
}

Step RequestHandler::handle(HttpRequest &req)
{
	// Govde limiti conf'tan; direktif verilmemisse (0) sinirsiz sayilir
	size_t limit = bodyLimit(req);
	if(limit != 0 && req.body.size() > limit)
	{
		Step x;
		x.kind = Step::RESPOND;
		x.response = error(413, server);
		return x;
	}

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
	if (best->allowed_methods.empty()) {
        allowed = true;
    } else {
        for(size_t i = 0 ; i < best->allowed_methods.size(); i++)
        {
            // HEAD bilerek GET'e baglanmadi: tester_42 "/ sadece GET"te HEAD icin 405 bekliyor
            if(best->allowed_methods[i] == req.method)
            {
                allowed = true;
                break;
            }
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

	std::string ext = utils::getExtension(path);
	std::map<std::string, std::string>::const_iterator cgi_it = best->cgi_pass.find(ext);
	if(cgi_it != best->cgi_pass.end() && !cgi_it->second.empty())
	{
		struct Step x = Step();
		std::string scriptPath = resolveDiskPath(*best, path);

		// resmi tester: POST'ta .bla, dosya diskte olmasa da CGI'ya gider.
		// GET'te olmayan script 404 kalir (b4 beklentisi).
		if (req.method != "POST" && access(scriptPath.c_str(), F_OK) != 0)
		{
			x.kind = Step::RESPOND;
			x.response = error(404, server);
			return x;
		}

		std::map<std::string, std::string> env_variables = buildCgiEnv(req, *best, *server, scriptPath);
		int output_pipe[2];
		int input_pipe[2];
		if (pipe(output_pipe) == -1)
		{
			x.kind = Step::RESPOND;
			x.response = error(500, server);
			return x;
		}
		if (pipe(input_pipe) == -1)
		{
			close(output_pipe[0]);
			close(output_pipe[1]);
			x.kind = Step::RESPOND;
			x.response = error(500, server);
			return x;
		}
		int id = fork();
		if(id == -1)
		{
			close(output_pipe[0]);
			close(output_pipe[1]);
			close(input_pipe[0]);
			close(input_pipe[1]);
			x.kind = Step::RESPOND;
			x.response = error(500, server);
			return x;
		}
		else if(id == 0)
		{
			close(output_pipe[0]); //child will not read so close it !!!
			close(input_pipe[1]); //child will not write anything so close it!!
		 	ResponseHandler::handle_cgi_child(output_pipe[1], input_pipe[0], env_variables, cgi_it->second);
		}
		else //parent processs
		{
			close(output_pipe[1]); //parent will not write anything so close it!!
			close(input_pipe[0]); //parent will not read anything so close it!!
			int fl = fcntl(input_pipe[1], F_GETFL);
			if (fl != -1)
				fcntl(input_pipe[1], F_SETFL, fl | O_NONBLOCK);

			cgiActive = true;
			cgiPid = id;
			cgiPipeFd = output_pipe[0];
			cgiStdinFd = input_pipe[1];
			cgiBody.swap(req.body);
			cgiInOffset = 0;
			cgiBuffer.clear();

			struct WatchFd y = WatchFd();
			fl = fcntl(output_pipe[0], F_GETFL);
			if (fl != -1)
				fcntl(output_pipe[0], F_SETFL, fl | O_NONBLOCK);
			y.fd = output_pipe[0];
			y.events = POLLIN;
			x.watch.push_back(y);

			// govde olay dongusunde yazilir; ayni anda CGI ciktisi okunabilsin ki
			// buyuk govdelerde iki boru birbirini kilitlemesin
			struct WatchFd in = WatchFd();
			in.fd = input_pipe[1];
			in.events = POLLOUT;
			x.watch.push_back(in);
			x.kind = Step::WAIT;
		}
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


Step RequestHandler::onReady(int fd, short revents)
{
	struct Step x;
	x.kind = Step::WAIT;

	if (fd == cgiStdinFd)
	{
		if (revents & (POLLERR | POLLHUP)) // CGI artik okumuyor (öldü/çikti)
		{
			closeCgiStdin(x);
			return x;
		}
		if (revents & POLLOUT)
		{
			size_t remaining = cgiBody.size() - cgiInOffset;
			if (remaining > 0)
			{
				ssize_t n = write(cgiStdinFd, cgiBody.data() + cgiInOffset, remaining);
				if (n > 0)
					cgiInOffset += static_cast<size_t>(n);
			}
			if (cgiInOffset >= cgiBody.size()) // gövde bitti: CGI EOF görecek
				closeCgiStdin(x);
		}
		return x;
	}

	// ayni epoll turunda gelebilen bayat (kapanmis) fd olaylari: yok say
	if (!cgiActive || fd != cgiPipeFd)
		return x;

	if (revents & POLLIN)
	{
		char buf[4096];
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n > 0)
		{
			cgiBuffer.append(buf, static_cast<size_t>(n));
		}
		else if (n == 0)
		{
			// Output pipe closed: CGI finished
			close(fd);
			closeCgiStdin(x);
			cgiActive = false;
			cgiPipeFd = -1;
			if (cgiPid != -1)
			{
				waitpid(cgiPid, NULL, 0); // child process now dead, not a zombie
				cgiPid = -1;
			}
			x.kind = Step::RESPOND;
			if (cgiBuffer.empty())
				x.response = error(502, server); // CGI produced no output (likely execve failed)
			else
				parseCgiOutput(cgiBuffer, x.response);
		}
		else
		{
			// read returned -1; without errno we cannot distinguish EAGAIN from a
			// real error. We return WAIT and let the event loop tell us again.
			return x;
		}
	}
	else if (revents & (POLLERR | POLLHUP)) // fd error or pipe hang up (closed)
	{
		if(!(cgiBuffer.empty())) //there is a tricky situation in this case POLLHUP means pipe closed but there will remain cgi buffer. But kernel sends POLLHUP instaead of PULLIN so there is valid data in there but we just did the error in this if. So i fixed it
			parseCgiOutput(cgiBuffer, x.response);
		else 
			x.response = error(502, server);
		close(fd);
		closeCgiStdin(x);
		cgiActive = false;
		cgiPipeFd = -1;
		if (cgiPid != -1)
		{
			waitpid(cgiPid, NULL, 0);
			cgiPid = -1;
		}
		x.kind = Step::RESPOND;
	}

	return x;
}
