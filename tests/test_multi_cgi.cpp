// Multi-CGI parser testi
//   make multi_cgi
//
// Ayni location'da birden fazla cgi satiri parse edilip
// dogru interpreter ile eslestiriliyor mu kontrol eder.

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>
#include "../includes/core/ConfigParser.hpp"
#include "../includes/http/RequestHandler.hpp"

static void writeFile(const std::string& path, const std::string& content)
{
	std::ofstream f(path.c_str());
	f << content;
}

static void resetFixtures()
{
	mkdir("html/cgi", 0755);

	writeFile("html/cgi/multi_test.py",
		"#!/usr/bin/env python3\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"PYTHON_CGI_OK\")\n");

	writeFile("html/cgi/multi_test.php",
		"<?php\n"
		"echo \"PHP_CGI_OK\\n\";\n"
		"?>\n");
}

static HttpResponse runCgi(const ServerConfig& srv, HttpRequest& req)
{
	RequestHandler handler(srv);
	Step step = handler.handle(req);

	if (step.kind != Step::WAIT || step.watch.empty())
	{
		std::cout << "Expected WAIT for " << req.path
			<< ", got kind=" << step.kind
			<< " status=" << step.response.status << "\n";
		HttpResponse err;
		err.status = -1;
		return err;
	}

	int fd = step.watch[0].fd;
	Step ready;
	int tries = 0;
	do
	{
		ready = handler.onReady(fd, POLLIN);
		if (ready.kind == Step::WAIT)
			usleep(10000);
	} while (ready.kind == Step::WAIT && ++tries < 500); // 5 sn: CI'da php-cgi'in ilk acilisi 1 sn'yi asabiliyor

	if (ready.kind != Step::RESPOND)
	{
		std::cout << "CGI did not finish for " << req.path << "\n";
		HttpResponse err;
		err.status = -1;
		return err;
	}
	return ready.response;
}

static bool contains(const std::string& text, const std::string& part)
{
	return text.find(part) != std::string::npos;
}

static std::string makeConf(const std::string& order)
{
	char cwd[4096];
	if (!getcwd(cwd, sizeof(cwd)))
		cwd[0] = '\0';
	std::string root = std::string(cwd) + "/html";

	std::string pairs;
	if (order == "php_first")
	{
		pairs =
			"        cgi .php /usr/bin/php-cgi;\n"
			"        cgi .py /usr/bin/python3;\n";
	}
	else
	{
		pairs =
			"        cgi .py /usr/bin/python3;\n"
			"        cgi .php /usr/bin/php-cgi;\n";
	}

	std::ostringstream oss;
	oss << "server {\n"
		<< "    listen 127.0.0.1:8080;\n"
		<< "    server_name localhost;\n"
		<< "    client_max_body_size 1000000;\n"
		<< "    location / {\n"
		<< "        root " << root << ";\n"
		<< "        allow_methods GET POST;\n"
		<< "    }\n"
		<< "    location /cgi {\n"
		<< "        root " << root << ";\n"
		<< "        allow_methods GET POST;\n"
		<< pairs
		<< "    }\n"
		<< "}\n";
	return oss.str();
}

static ServerConfig parseConf(const std::string& content)
{
	writeFile("multi_cgi_test.conf", content);
	ConfigParser parser;
	Config cfg = parser.parse_config_file("multi_cgi_test.conf");
	if (cfg.servers.empty())
	{
		std::cerr << "No server parsed\n";
		std::exit(1);
	}
	return cfg.servers[0];
}

static bool testOrder(const std::string& order)
{
	std::cout << "\n--- Testing " << order << " ---\n";
	ServerConfig srv = parseConf(makeConf(order));

	HttpRequest req;
	req.version = "HTTP/1.1";
	req.headers["host"] = "localhost";
	req.remote_addr = "127.0.0.1";
	req.server_port = 8080;
	req.method = "GET";
	req.query = "";
	req.body = "";

	req.path = "/cgi/multi_test.py";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "PYTHON_CGI_OK"))
		{
			std::cout << order << " .py failed: status=" << res.status
				<< " body=" << res.body << "\n";
			return false;
		}
	}

	req.path = "/cgi/multi_test.php";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "PHP_CGI_OK"))
		{
			std::cout << order << " .php failed: status=" << res.status
				<< " body=" << res.body << "\n";
			return false;
		}
	}

	return true;
}

int main()
{
	char cwd[4096];
	if (!getcwd(cwd, sizeof(cwd)))
		return 1;
	resetFixtures();

	if (!testOrder("php_first"))
		return 1;
	if (!testOrder("py_first"))
		return 1;

	std::cout << "\nMulti-CGI parser tests passed\n";
	return 0;
}
