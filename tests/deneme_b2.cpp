// B2 oyun alani. Test degil: istegi elle doldur, ciplak cevabi ekranda gor.
//   make deneme
// main() icindeki blokları serbestce degistir, yorum satirina al, yenisini ekle.

#include <iostream>
#include "RequestHandler.hpp"

// Kok dizin "html". Location'lar: "/" (GET POST, autoindex acik)
static ServerConfig makeServer()
{
	ServerConfig srv;
	srv.root = "html";
	srv.index = "index.html";
	srv.client_max_body_size = 1000000;

	// Hata sayfalari. Buradaki yol URL mi disk yolu mu? Karari sen veriyorsun:
	// dosyalar html/errors/ altinda duruyor, yani error() URL kabul edip root
	// ile birlestirirse calisir. Disk yolu kabul edecekse "html/errors/..." yaz.
	srv.error_pages[404] = "/errors/404.html";
	srv.error_pages[403] = "/errors/403.html";
	// 405 icin bilerek kayit yok: gomulu HTML'in calistigini gormek icin.

	LocationConfig root;
	root.path = "/";
	root.root = "html";
	root.index = "index.html";
	root.autoindex = true;
	root.client_max_body_size = 1000000;
	root.redirect_code = 0;
	root.allowed_methods.push_back("GET");
	root.allowed_methods.push_back("POST");
	srv.locations.push_back(root);

	// Index dosyasi olmayan, autoindex kapali dizin -> 403
	LocationConfig gizli = root;
	gizli.path = "/gizli";
	gizli.autoindex = false;
	gizli.index = "bulunmayan.html";
	srv.locations.push_back(gizli);

	return srv;
}

static void run(const ServerConfig& srv, const HttpRequest& req)
{
	RequestHandler handler(srv);
	Step step = handler.handle(req);

	std::cout << "\n>>> " << req.method << " " << req.path << "\n";
	if (step.kind == Step::WAIT)
	{
		std::cout << "(WAIT, " << step.watch.size() << " fd)\n";
		return;
	}
	std::cout << step.response.serialize() << "\n";
}

int main()
{
	ServerConfig srv = makeServer();

	HttpRequest req;
	req.version = "HTTP/1.1";
	req.headers["host"] = "localhost";
	req.remote_addr = "127.0.0.1";
	req.server_port = 8080;
// 
// 	req.method = "GET";
// 	req.path = "/index.html";
// 	run(srv, req);
	
	// 
	// req.method = "GET";
	// req.path = "/asd";
	// run(srv, req);

	// --- hata sayfalari ---

	// 404: config'de kayit var -> html/errors/404.html icerigi gelmeli
	// req.method = "GET";
	// req.path = "/yok.html";
	// run(srv, req);

	// 403: index yok, autoindex kapali -> html/errors/403.html icerigi gelmeli
	req.method = "GET";
	req.path = "/gizli/";
	run(srv, req);


	req.method = "GET";
	req.path = "/gizli";
	run(srv, req);
// 
	// // 405: config'de kayit YOK -> gomulu HTML gelmeli, Allow header'i da olmali
	req.method = "GET";
	req.path = "/index.html/";
	run(srv, req);

	req.method = "GET";
	req.path = "/errors/";
	run(srv, req);
// 
// 	// 400: root disina cikma denemesi
// 	req.method = "GET";
// 	req.path = "/../Makefile";
// 	run(srv, req);

	// req.method = "GET";
	// req.path = "/";
	// run(srv, req);

	// req.method = "GET";
	// req.path = "/yok.html";
	// run(srv, req);

	// req.method = "DELETE";
	// req.path = "/index.html";
	// run(srv, req);

	return 0;
}
