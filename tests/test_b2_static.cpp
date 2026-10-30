// B2 kontrolleri: statik dosya, MIME, index, autoindex, hata sayfalari.
//   make b2
//
// Her senaryo cevabi ekrana basar ve beklenen degerleri dogrular; beklenmeyen
// bir sonucta program 1 ile cikar. Fixture'lar her calistirmada kurulur.

#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <cstdio>
#include "RequestHandler.hpp"

// Kok dizin "html". Location'lar: "/" (GET POST, autoindex acik), "/gizli"
static ServerConfig makeServer()
{
	ServerConfig srv;
	srv.root = "html";
	srv.index.push_back("index.html");
	srv.client_max_body_size = 1000000;

	// Hata sayfalari URL bicimindedir; error() bunlari root ile birlestirir.
	srv.error_pages[404] = "/errors/404.html";
	srv.error_pages[403] = "/errors/403.html";
	// 405 icin bilerek kayit yok: gomulu HTML'in calistigini gormek icin.

	LocationConfig root;
	root.path = "/";
	root.root = "html";
	root.index.push_back("index.html");
	root.autoindex = true;
	root.upload_enable = false;
	root.client_max_body_size = 1000000;
	root.redirect_code = 0;
	root.allowed_methods.push_back("GET");
	root.allowed_methods.push_back("POST");
	srv.locations.push_back(root);

	// Index dosyasi olmayan, autoindex kapali dizin -> 404
	LocationConfig gizli = root;
	gizli.path = "/gizli";
	gizli.autoindex = false;
	gizli.index.push_back("bulunmayan.html");
	srv.locations.push_back(gizli);

	// Yonlendirme
	LocationConfig eski = root;
	eski.path = "/eski";
	eski.redirect_code = 301;
	eski.redirect_target = "/index.html";
	srv.locations.push_back(eski);

	return srv;
}

static void write(const std::string& path, const std::string& data)
{
	std::ofstream out(path.c_str(), std::ios::binary);
	out << data;
}

// Her calistirmada temiz baslangic
static void resetFixtures()
{
	mkdir("html", 0755);
	mkdir("html/b2", 0755);
	write("html/b2/bos.txt", "");
	write("html/b2/metin.txt", "merhaba");
	write("html/b2/resim.png", std::string("\x89PNG\r\n\x1a\n\0\0ORTA\0NULLXX", 20));
	write("html/b2/BUYUK.PNG", "x");
	write("html/b2/uzantisiz", "veri");
	mkdir("html/b2/listelenecek", 0755);
	write("html/b2/listelenecek/a.txt", "a");
	write("html/b2/listelenecek/b.txt", "b");
	mkdir("html/b2/listelenecek/altklasor", 0755);
	write("html/b2/izinsiz.html", "gizli");
	chmod("html/b2/izinsiz.html", 0000);
}

static void cleanFixtures()
{
	chmod("html/b2/izinsiz.html", 0644);
	std::remove("html/b2/izinsiz.html");
	std::remove("html/b2/listelenecek/altklasor");
	std::remove("html/b2/listelenecek/a.txt");
	std::remove("html/b2/listelenecek/b.txt");
	std::remove("html/b2/listelenecek");
	std::remove("html/b2/uzantisiz");
	std::remove("html/b2/BUYUK.PNG");
	std::remove("html/b2/resim.png");
	std::remove("html/b2/metin.txt");
	std::remove("html/b2/bos.txt");
	std::remove("html/b2");
}

static HttpResponse run(const ServerConfig& srv, const std::string& method, const std::string& path)
{
	HttpRequest req;
	req.version = "HTTP/1.1";
	req.headers["host"] = "localhost";
	req.remote_addr = "127.0.0.1";
	req.server_port = 8080;
	req.method = method;
	req.path = path;

	RequestHandler handler(srv);
	Step step = handler.handle(req);

	std::cout << "\n>>> " << method << " " << path << "\n";
	if (step.kind == Step::WAIT)
	{
		std::cout << "(WAIT, " << step.watch.size() << " fd)\n";
		return step.response;
	}
	std::cout << step.response.serialize() << "\n";
	return step.response;
}

static std::string header(const HttpResponse& res, const std::string& name)
{
	std::map<std::string, std::string>::const_iterator it = res.headers.find(name);
	if (it == res.headers.end())
		return "";
	return it->second;
}

static bool contains(const std::string& text, const std::string& part)
{
	return text.find(part) != std::string::npos;
}

int main()
{
	resetFixtures();
	ServerConfig srv = makeServer();
	HttpResponse res;

	// --- normal dosya: icerik ve MIME ---
	res = run(srv, "GET", "/b2/metin.txt");
	if (res.status != 200 || res.body != "merhaba"
		|| !contains(header(res, "Content-Type"), "text/plain"))
		return 1;

	// --- ikili dosya: null byte'lar korunmali ---
	res = run(srv, "GET", "/b2/resim.png");
	if (res.status != 200 || res.body.size() != 20
		|| header(res, "Content-Type") != "image/png")
		return 1;

	// --- bos dosya hata degil ---
	res = run(srv, "GET", "/b2/bos.txt");
	if (res.status != 200 || !res.body.empty())
		return 1;

	// --- uzanti buyuk harf olsa da taninmali ---
	res = run(srv, "GET", "/b2/BUYUK.PNG");
	if (res.status != 200 || header(res, "Content-Type") != "image/png")
		return 1;

	// --- uzantisiz dosya -> octet-stream ---
	res = run(srv, "GET", "/b2/uzantisiz");
	if (res.status != 200 || header(res, "Content-Type") != "application/octet-stream")
		return 1;

	// --- dizin + index dosyasi ---
	res = run(srv, "GET", "/");
	if (res.status != 200 || !contains(res.body, "<"))
		return 1;

	// --- dizin, index yok, autoindex acik -> liste ---
	res = run(srv, "GET", "/b2/listelenecek/");
	if (res.status != 200
		|| !contains(res.body, "a.txt") || !contains(res.body, "b.txt")
		|| !contains(res.body, "altklasor/")            // dizinler '/' ile biter
		|| !contains(res.body, "Index of /b2/listelenecek")) // URL gosterilir, disk yolu degil
		return 1;

	// --- dizin istegi '/' ile bitmiyorsa 301 ---
	res = run(srv, "GET", "/b2/listelenecek");
	if (res.status != 301 || header(res, "Location") != "/b2/listelenecek/")
		return 1;

	// --- duzenli dosya '/' ile istenirse 404 ---
	res = run(srv, "GET", "/b2/metin.txt/");
	if (res.status != 404)
		return 1;

	// --- dizin, index yok, autoindex kapali -> 404 (42 tester /directory/Yeah bunu bekliyor) + config'deki hata sayfasi ---
	res = run(srv, "GET", "/gizli/");
	if (res.status != 404 || !contains(res.body, "error_page 404"))
		return 1;

	// --- olmayan dosya -> 404 + config'deki hata sayfasi ---
	res = run(srv, "GET", "/yok.html");
	if (res.status != 404 || !contains(res.body, "error_page 404"))
		return 1;

	// --- izinsiz dosya -> 403 ---
	res = run(srv, "GET", "/b2/izinsiz.html");
	if (res.status != 403)
		return 1;

	// --- izin verilmeyen metot -> 405 + Allow, config'de kayit yok -> gomulu HTML ---
	res = run(srv, "DELETE", "/index.html");
	if (res.status != 405
		|| !contains(header(res, "Allow"), "GET")
		|| !contains(res.body, "<h1>405</h1>"))
		return 1;

	// --- yonlendirme ---
	res = run(srv, "GET", "/eski");
	if (res.status != 301 || header(res, "Location") != "/index.html")
		return 1;

	// --- root disina cikma -> 400 ---
	res = run(srv, "GET", "/../Makefile");
	if (res.status != 400)
		return 1;

	// --- root icinde kalan ".." ve "." serbest ---
	res = run(srv, "GET", "/b2/../b2/./metin.txt");
	if (res.status != 200 || res.body != "merhaba")
		return 1;

	// --- serialize: durum satiri, CRLF, bos satir, Content-Length ---
	res = run(srv, "GET", "/b2/metin.txt");
	std::string raw = res.serialize();
	if (raw.compare(0, 17, "HTTP/1.1 200 OK\r\n") != 0
		|| !contains(raw, "Content-Length: 7\r\n")
		|| !contains(raw, "\r\n\r\nmerhaba"))
		return 1;

	// --- HEAD: Content-Length GET'teki gibi, govde yok ---
	raw = res.serialize(true);
	if (!contains(raw, "Content-Length: 7\r\n")
		|| raw.compare(raw.size() - 4, 4, "\r\n\r\n") != 0)
		return 1;

	cleanFixtures();
	return 0;
}
