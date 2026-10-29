// B4 CGI testi
//   make b4
//
// CGI location'ina gelen isteklerin fork/execve ile calistirilip
// output pipe'tan cevabin okunup HTTP yanitina donusturuldugunu kontrol eder.

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include <poll.h>
#include "../includes/RequestHandler.hpp"

static ServerConfig makeServer()
{
	ServerConfig srv;
	srv.root = "html";
	srv.index.push_back("index.html");
	srv.client_max_body_size = 1000000;
	srv.server_name.push_back("localhost");

	LocationConfig root;
	root.path = "/";
	root.root = "html";
	root.index.push_back("index.html");
	root.allowed_methods.push_back("GET");
	root.allowed_methods.push_back("POST");
	srv.locations.push_back(root);

	LocationConfig cgi = root;
	cgi.path = "/cgi";
	cgi.root = "html";
	cgi.allowed_methods.clear();
	cgi.allowed_methods.push_back("GET");
	cgi.allowed_methods.push_back("POST");
	cgi.cgi_pass[".py"] = "/usr/bin/python3";
	srv.locations.push_back(cgi);

	return srv;
}

static void writeFile(const std::string& path, const std::string& content)
{
	std::ofstream f(path.c_str());
	f << content;
}

static std::string header(const HttpResponse& res, const std::string& key)
{
	std::map<std::string, std::string>::const_iterator it = res.headers.find(key);
	if (it != res.headers.end())
		return it->second;
	return "";
}

static bool contains(const std::string& text, const std::string& part)
{
	return text.find(part) != std::string::npos;
}

static void resetFixtures()
{
	mkdir("html/cgi", 0755);

	// Basit GET cevabi veren script
	writeFile("html/cgi/test.py",
		"#!/usr/bin/env python3\n"
		"print(\"Content-Type: text/html\")\n"
		"print()\n"
		"print(\"<html><body><h1>CGI OK</h1></body></html>\")\n");

	// Query string'i ekrana basan script
	writeFile("html/cgi/echo.py",
		"#!/usr/bin/env python3\n"
		"import os\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"QUERY_STRING=\" + os.environ.get(\"QUERY_STRING\", \"\"))\n");

	// POST body'sini stdin'den okuyup geri basan script
	writeFile("html/cgi/post.py",
		"#!/usr/bin/env python3\n"
		"import os, sys\n"
		"length = int(os.environ.get(\"CONTENT_LENGTH\", \"0\"))\n"
		"body = sys.stdin.read(length)\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"received:\" + body)\n");

	// Status header ile hata donen script
	writeFile("html/cgi/error.py",
		"#!/usr/bin/env python3\n"
		"print(\"Status: 404 Not Found\")\n"
		"print(\"Content-Type: text/html\")\n"
		"print()\n"
		"print(\"<h1>not found</h1>\")\n");

	// Custom Content-Type donen script
	writeFile("html/cgi/ctype.py",
		"#!/usr/bin/env python3\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"plain text\")\n");

	// Calisma dizinini test eden script
	writeFile("html/cgi/data.txt", "relative data\n");
	writeFile("html/cgi/cwd.py",
		"#!/usr/bin/env python3\n"
		"with open(\"data.txt\") as f:\n"
		"    data = f.read().strip()\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"cwd=\" + data)\n");

	// Buyuk cikti ureten script (4096'dan fazla, birden fazla read gerektirir)
	writeFile("html/cgi/big.py",
		"#!/usr/bin/env python3\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"x\" * 10000)\n");

	// Buyuk POST govdesini stdin'den okuyup uzunlugunu donen script
	writeFile("html/cgi/sink.py",
		"#!/usr/bin/env python3\n"
		"import os, sys\n"
		"length = int(os.environ.get('CONTENT_LENGTH', '0'))\n"
		"body = sys.stdin.read(length)\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print('len=' + str(len(body)))\n");

	// Hicbir sey basmayip execve sonrasi hemen cikan script (502 beklenir)
	writeFile("html/cgi/empty.py",
		"#!/usr/bin/env python3\n"
		"pass\n");

	// Ortam degiskenlerini kontrol eden script
	writeFile("html/cgi/env.py",
		"#!/usr/bin/env python3\n"
		"import os\n"
		"print(\"Content-Type: text/plain\")\n"
		"print()\n"
		"print(\"METHOD=\" + os.environ.get(\"REQUEST_METHOD\", \"\"))\n"
		"print(\"CTYPE=\" + os.environ.get(\"CONTENT_TYPE\", \"\"))\n");

	// CGI location'i altindaki statik dosya
	writeFile("html/cgi/page.html",
		"<html><body>static file under cgi</body></html>\n");
}

static bool stdinClosed(const Step& s, int fd)
{
	for (size_t i = 0; i < s.unwatch.size(); i++)
		if (s.unwatch[i] == fd)
			return true;
	return false;
}

static HttpResponse runCgi(const ServerConfig& srv, HttpRequest& req)
{
	std::cout << "\n>>> " << req.method << " " << req.path << "\n";

	RequestHandler handler(srv);
	Step step = handler.handle(req);

	if (step.kind != Step::WAIT || step.watch.empty())
	{
		std::cout << "Expected WAIT, got kind=" << step.kind
			<< " status=" << step.response.status << "\n";
		HttpResponse err;
		err.status = -1;
		return err;
	}

	int outFd = -1;
	int inFd = -1;
	for (size_t i = 0; i < step.watch.size(); i++)
	{
		if (step.watch[i].events & POLLIN)
			outFd = step.watch[i].fd;
		if (step.watch[i].events & POLLOUT)
			inFd = step.watch[i].fd;
	}

	Step ready;
	int tries = 0;
	bool writing = (inFd != -1);
	do
	{
		if (writing)
		{
			ready = handler.onReady(inFd, POLLOUT);
			if (stdinClosed(ready, inFd))
				writing = false;
			else if (ready.kind == Step::WAIT)
				usleep(1000);
		}
		else
		{
			ready = handler.onReady(outFd, POLLIN);
			if (ready.kind == Step::WAIT)
				usleep(10000);
		}
	} while (ready.kind == Step::WAIT && ++tries < 1000);

	if (ready.kind != Step::RESPOND)
	{
		std::cout << "CGI did not finish in time\n";
		HttpResponse err;
		err.status = -1;
		return err;
	}

	std::cout << ready.response.serialize() << "\n";
	return ready.response;
}

static HttpResponse runStatic(const ServerConfig& srv, HttpRequest& req)
{
	std::cout << "\n>>> " << req.method << " " << req.path << " (static)\n";

	RequestHandler handler(srv);
	Step step = handler.handle(req);

	if (step.kind != Step::RESPOND)
	{
		std::cout << "Expected RESPOND, got WAIT\n";
		HttpResponse err;
		err.status = -1;
		return err;
	}

	std::cout << step.response.serialize() << "\n";
	return step.response;
}

int main()
{
	resetFixtures();
	ServerConfig srv = makeServer();

	HttpRequest req;
	req.version = "HTTP/1.1";
	req.headers["host"] = "localhost";
	req.remote_addr = "127.0.0.1";
	req.server_port = 8080;

	// --- Basit GET CGI istegi ---
	req.method = "GET";
	req.path = "/cgi/test.py";
	req.query = "";
	req.body = "";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "CGI OK"))
			return 1;
	}

	// --- Query string'in env olarak script'e ulastigini kontrol et ---
	req.method = "GET";
	req.path = "/cgi/echo.py";
	req.query = "isim=ali";
	req.body = "";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "QUERY_STRING=isim=ali"))
			return 1;
	}

	// --- POST body'nin stdin uzerinden script'e ulastigini kontrol et ---
	req.method = "POST";
	req.path = "/cgi/post.py";
	req.query = "";
	req.headers["content-type"] = "text/plain";
	req.body = "merhaba cgi";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "received:merhaba cgi"))
			return 1;
	}
	req.headers.erase("content-type");
	req.body = "";

	// --- Status header'i ile 404 donen CGI ---
	req.method = "GET";
	req.path = "/cgi/error.py";
	req.query = "";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 404 || !contains(res.body, "not found"))
			return 1;
	}

	// --- Custom Content-Type basan CGI ---
	req.method = "GET";
	req.path = "/cgi/ctype.py";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || header(res, "Content-Type") != "text/plain"
			|| !contains(res.body, "plain text"))
			return 1;
	}

	// --- Calisma dizini dogru ayarlanmali ---
	req.method = "GET";
	req.path = "/cgi/cwd.py";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "cwd=relative data"))
			return 1;
	}

	// --- Buyuk cikti (birden fazla read) ---
	req.method = "GET";
	req.path = "/cgi/big.py";
	{
		HttpResponse res = runCgi(srv, req);
		// Python print() sonuna yeni satir ekler: 10000 + 1 = 10001
		if (res.status != 200 || res.body.size() != 10001)
			return 1;
	}

	// --- Buyuk POST govdesi (boru tamponundan fazla, kismi yazma) ---
	req.method = "POST";
	req.path = "/cgi/sink.py";
	req.query = "";
	req.headers["content-type"] = "text/plain";
	req.body = std::string(200000, 'a');
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200 || !contains(res.body, "len=200000"))
			return 1;
	}
	req.headers.erase("content-type");
	req.body.clear();

	// --- Hic cikti uretmeyen script -> 502 ---
	req.method = "GET";
	req.path = "/cgi/empty.py";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 502)
			return 1;
	}

	// --- Olmayan CGI script -> 404 ---
	req.method = "GET";
	req.path = "/cgi/missing.py";
	{
		HttpResponse res = runStatic(srv, req);
		if (res.status != 404)
			return 1;
	}

	// --- Header anahtarlari buyuk/kucuk harf duyarsiz olmali ---
	req.method = "POST";
	req.path = "/cgi/env.py";
	req.query = "";
	req.headers["content-type"] = "text/plain"; // kucuk harfli key
	req.body = "";
	{
		HttpResponse res = runCgi(srv, req);
		if (res.status != 200
			|| !contains(res.body, "METHOD=POST")
			|| !contains(res.body, "CTYPE=text/plain"))
			return 1;
	}
	req.headers.erase("content-type");

	// --- CGI location'i altinda uzantisi .py olmayan dosya statik sunulmali ---
	req.method = "GET";
	req.path = "/cgi/page.html";
	{
		HttpResponse res = runStatic(srv, req);
		if (res.status != 200 || !contains(res.body, "static file under cgi"))
			return 1;
	}

	// --- Izin verilmeyen method -> 405 ---
	req.method = "DELETE";
	req.path = "/cgi/test.py";
	{
		HttpResponse res = runStatic(srv, req);
		if (res.status != 405)
			return 1;
	}

	std::cout << "\nB4 CGI tests passed\n";
	return 0;
}
