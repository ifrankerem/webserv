#include "../../includes/http/RequestHandler.hpp"
#include "../../includes/core/Config.hpp"
#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/http/modules/ResponseHandler.hpp"
#include "../../includes/core/Utils.hpp"
#include <csignal>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <ostream>
#include <poll.h>
#include <string>
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
			|| utils::endsWithSlash(loc.path)
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
		return utils::joinPath(loc.alias, path.substr(loc.path.size()));
	if (loc.root.empty())
		return utils::joinPath(server->root, path);
	return utils::joinPath(loc.root, path);
}

std::string RequestHandler::resolveUploadPath(const LocationConfig& loc, const std::string& path)
{
	return utils::joinPath(loc.upload_store, path.substr(loc.path.size()));
}

static std::map<std::string, std::string> buildCgiEnv(const HttpRequest& req, const ServerConfig& srv, const std::string& scriptPath)
{
	std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = req.method;
	env["SCRIPT_NAME"] = req.path;
	env["SCRIPT_FILENAME"] = scriptPath.substr(scriptPath.rfind('/') + 1);
	env["PATH_INFO"] = req.path;
	env["REQUEST_URI"] = req.path;
	env["PATH_TRANSLATED"] = "";
	env["QUERY_STRING"] = req.query;
	env["SERVER_PROTOCOL"] = req.version;
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["CONTENT_TYPE"] = utils::getHeader(req.headers, "content-type");
	env["CONTENT_LENGTH"] = utils::toString(req.body.size());
	std::string host_name = utils::getHeader(req.headers, "host");
	if(srv.server_name.size() != 0)
	{
		env["SERVER_NAME"] = srv.server_name[0]; //default is the first one
		for(size_t i = 0; i < srv.server_name.size();i++)
		{
			if(host_name == srv.server_name[i])
			{
				env["SERVER_NAME"] = host_name;
				break;
			}
		}
	}
	env["SERVER_PORT"] = utils::toString(static_cast<size_t>(req.server_port));
	env["REMOTE_ADDR"] = req.remote_addr;
	env["REDIRECT_STATUS"] = "200";

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
	return env;
}

static bool parseCgiOutput(std::string& raw, HttpResponse& response)
{
	response.status = 0; // 0 = script Status yazmadi; asagida 200 ya da 302 olur

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
	if (headersPart.empty())
		return false;
	response.body.swap(raw);

	std::istringstream lines(headersPart);
	std::string line;
	while (std::getline(lines, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return false;
		std::string key = line.substr(0, colon);
		std::string value = utils::trim(line.substr(colon + 1));
		std::string lower = utils::toLower(key); // header adlari buyuk/kucuk harfe duyarsiz: php "Content-type" yazar
		if (lower == "status")
			response.status = std::atoi(value.c_str());
		else if (lower == "content-type")
			response.headers["Content-Type"] = value;
		else if (lower == "location")
			response.headers["Location"] = value;
		else if(lower == "set-cookie")
			response.set_cookies.push_back(value);
		else
			response.headers[key] = value;
	}

	// RFC 3875 6.2.3: Status'suz Location bir yonlendirmedir
	if (response.status == 0)
	{
		if (response.headers.count("Location"))
			response.status = 302;
		else
			response.status = 200;
	}

	if (response.headers.find("Content-Type") == response.headers.end())
		response.headers["Content-Type"] = "text/html; charset=UTF-8";

	return true;

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
	std::string path = server->root + it->second;
	if (loc != NULL)
		path = tmp.resolveDiskPath(*loc, it->second);
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
	Step x;
	x.kind = Step::RESPOND;

	std::string path;
	if (!validateRequest(req, path, x))
		return x;

	const LocationConfig *best = applyLocationRules(req, path, x);
	if (best == NULL)
		return x;

	return execute(req, path, best);
}

bool RequestHandler::validateRequest(HttpRequest &req, std::string &path, Step &x)
{
	// Govde limiti conf'tan; direktif verilmemisse (0) sinirsiz sayilir
	size_t limit = bodyLimit(req);
	if(limit != 0 && req.body.size() > limit)
	{
		x.response = error(413, server);
		return false;
	}

	if (!normalizePath(utils::decodePath(req.path), path))
	{
		x.response = error(400,server);
		return false;
	}
	return true;
}

const LocationConfig* RequestHandler::applyLocationRules(HttpRequest &req, const std::string &path, Step &x)
{
	const LocationConfig *best = findLocation(path);

	if(best == NULL)
	{
		//TODO conf has no location so maybe use Default Settings?
		x.response = error(404,server);
		return NULL;
	}

	bool allowed = false;
	if (best->allowed_methods.empty()) //all methods are allowed
		allowed = true;
	else if(std::find(best->allowed_methods.begin(), best->allowed_methods.end(), req.method) != best->allowed_methods.end())
		allowed = true;

	if (allowed == false)
	{
	    std::string allow;
	    for (size_t i = 0; i < best->allowed_methods.size(); i++)
	    {
	        if (i > 0)
	            allow += ", ";
	        allow += best->allowed_methods[i];
	    }

	    x.response = error(405,server);
	    x.response.headers["Allow"] = allow;
	    return NULL;
	}

	if(best->redirect_code != 0)
	{
		if(best->redirect_code / 100 == 3)
			x.response = ResponseHandler::redirect_handler(best->redirect_code,best->redirect_target);
		else
			x.response = error(best->redirect_code,server);
		return NULL;
	}
	return best;
}

Step RequestHandler::execute(HttpRequest &req, const std::string &path, const LocationConfig *best)
{
	std::string ext = utils::getExtension(path);
	std::map<std::string, std::string>::const_iterator cgi_it = best->cgi_pass.find(ext);
	if(cgi_it != best->cgi_pass.end() && !cgi_it->second.empty())
		return executeCgi(req, path, best, cgi_it->second);
	return executeMethod(req, path, best);
}

Step RequestHandler::executeCgi(HttpRequest &req, const std::string &path, const LocationConfig *best, const std::string &interpreter)
{
	struct Step x = Step();
	std::string scriptPath = resolveDiskPath(*best, path);

	if (req.method != "POST" && access(scriptPath.c_str(), F_OK) != 0)
	{
		x.kind = Step::RESPOND;
		x.response = error(404, server);
		return x;
	}

	std::map<std::string, std::string> env_variables = buildCgiEnv(req, *server, scriptPath);
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
	std::flush(std::cout); //flushing the cout because stdout will be pipe after this
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
	 	ResponseHandler::handle_cgi_child(output_pipe[1], input_pipe[0], env_variables, scriptPath, interpreter);
	}
	else //parent processs
	{
		close(output_pipe[1]); //parent will not write anything so close it!!
		close(input_pipe[0]); //parent will not read anything so close it!!
		fcntl(input_pipe[1], F_SETFL, O_NONBLOCK);


		cgiActive = true;
		cgiPid = id;
		cgiPipeFd = output_pipe[0];
		cgiStdinFd = input_pipe[1];
		cgiBody.swap(req.body);
		cgiInOffset = 0;
		cgiBuffer.clear();

		struct WatchFd out = WatchFd();
		out.fd = output_pipe[0];
		fcntl(output_pipe[0], F_SETFL, O_NONBLOCK);
		out.events = POLLIN;
		x.watch.push_back(out);

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

Step RequestHandler::executeMethod(HttpRequest &req, const std::string &path, const LocationConfig *best)
{
	Step x;
	x.kind = Step::RESPOND;
	std::string diskPath;
	//POST ve DELETE ayni klasorle calisir: hedefi tek yerde hesapla
	if ((req.method == "DELETE" || req.method == "POST") && !best->upload_store.empty())
		diskPath = resolveUploadPath(*best, path);
	else
		diskPath = resolveDiskPath(*best, path); // its final version of path root + req
	x.response = ResponseHandler::handle(diskPath, path, req, *best, *server);
	return x;
}

//UZERINDEN GEC


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
			cgiBuffer.append(buf, static_cast<size_t>(n));
		else if (n == 0)
		{
			// Output pipe closed: CGI finished
			close(fd);
			closeCgiStdin(x);
			cgiActive = false;
			cgiPipeFd = -1;
			if (cgiPid != -1)
			{
				pid_t return_pid = waitpid(cgiPid, NULL, WNOHANG); 
				if(return_pid == 0)
					kill(cgiPid,SIGKILL); //sdout closed but script not finished so
				waitpid(cgiPid, NULL, 0);
				cgiPid = -1;
			}
			x.kind = Step::RESPOND;
			if (cgiBuffer.empty())
				x.response = error(502, server); // CGI produced no output (likely execve failed)
			else
				if(parseCgiOutput(cgiBuffer, x.response) == false)
					x.response = error(502, server);
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
		{
			if(parseCgiOutput(cgiBuffer, x.response) == false)
				x.response = error(502, server);
		}
		else
			x.response = error(502, server);
		close(fd);
		closeCgiStdin(x);
		cgiActive = false;
		cgiPipeFd = -1;
		if (cgiPid != -1)
		{
			pid_t return_pid = waitpid(cgiPid, NULL, WNOHANG); 
			if(return_pid == 0)
				kill(cgiPid,SIGKILL); //sdout closed but script not finished so
			waitpid(cgiPid, NULL, 0);
			cgiPid = -1;
		}
		x.kind = Step::RESPOND;
	}

	return x;
}
