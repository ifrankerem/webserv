// B3 upload / DELETE kontrolleri.
//   make b3
//
// Her calistirmada html/upload/ icerigi sifirlanir, yoksa DELETE denemeleri
// ikinci calistirmada 404 doner ve kod bozuk sanilir.

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include "../includes/http/RequestHandler.hpp"

static bool fileExists(const std::string& path)
{
	std::ifstream file(path.c_str());
	return file.good();
}

static std::string fileContent(const std::string& path)
{
	std::ifstream file(path.c_str());
	std::istreambuf_iterator<char> first(file);
	std::istreambuf_iterator<char> last;
	return std::string(first, last);
}

static ServerConfig makeServer()
{
	ServerConfig srv;
	srv.root = "html";
	srv.index.push_back("index.html");
	srv.client_max_body_size = 1000000;
	srv.error_pages[404] = "/errors/404.html";
	srv.error_pages[403] = "/errors/403.html";

	LocationConfig root;
	root.path = "/";
	root.root = "html";
	root.index.push_back("index.html");
	root.autoindex = true;
	root.upload_enable = false;
	root.client_max_body_size = 1000000;
	root.redirect_code = 0;
	root.allowed_methods.push_back("GET");
	srv.locations.push_back(root);

	// Upload alani: uc metoda da izinli, kucuk body siniri.
	LocationConfig upload = root;
	upload.path = "/upload";
	upload.root = "html";
	upload.upload_store = "html/upload/klasor";
	upload.upload_enable = true;
	upload.allowed_methods.clear();
	upload.allowed_methods.push_back("GET");
	upload.allowed_methods.push_back("POST");
	upload.allowed_methods.push_back("DELETE");
	upload.client_max_body_size = 1000000;
	srv.locations.push_back(upload);

	return srv;
}

// Her calistirmada temiz baslangic
static void resetFixtures()
{
	mkdir("html/upload", 0755);
	mkdir("html/upload/klasor", 0755);
	std::ofstream("html/upload/klasor/silinecek.txt").put('x');
	mkdir("html/upload/klasor/altklasor", 0755);
	std::ofstream("html/upload/klasor/altklasor/korunacak.txt").put('y');
	std::ofstream("html/upload/klasor/2.txt").put('x');
	mkdir("html/upload/klasor/step", 0755);
	std::ofstream("html/upload/b3_escaped.txt").put('x');
	mkdir("html/upload/b3_outside", 0755);
	std::ofstream("html/upload/b3_outside/korunacak.txt").put('x');
	std::remove("html/upload/klasor/b3_link");
	symlink("../b3_outside", "html/upload/klasor/b3_link");
}

static int run(const ServerConfig& srv, HttpRequest& req)
{
	RequestHandler handler(srv);
	Step step = handler.handle(req);

	std::cout << "\n>>> " << req.method << " " << req.path;
	if (!req.body.empty())
		std::cout << "   (body " << req.body.size() << " byte)";
	std::cout << "\n";

	if (step.kind == Step::WAIT)
	{
		std::cout << "(WAIT, " << step.watch.size() << " fd)\n";
		return -1;
	}
	std::cout << step.response.serialize() << "\n";
	return step.response.status;
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

	// --- POST: duz govde (curl --data-binary gibi) ---
	// Dosya adi URL'den geliyor, dosya upload_store altinda olusmali.
	req.method = "POST";
	req.path = "/upload/not.txt";
	req.headers["content-type"] = "text/plain";
	req.body = "merhaba dunya";
	if (run(srv, req) != 201 || fileContent("html/upload/klasor/not.txt") != "merhaba dunya")
		return 1;

	// --- POST: ikili govde, ortasinda null byte ---
	// Null byte'ta kesilirse boyut tutmaz.
	req.path = "/upload/ikili.bin";
	req.headers["content-type"] = "application/octet-stream";
	req.body = std::string("AB\0CD", 5);
	if (run(srv, req) != 201 || fileContent("html/upload/klasor/ikili.bin").size() != 5)
		return 1;

	// --- POST: alt klasore yukleme, POST ve DELETE ayni yolu gostermeli ---
	req.path = "/upload/altklasor/yeni.txt";
	req.headers["content-type"] = "text/plain";
	req.body = "alt klasor";
	if (run(srv, req) != 201 || !fileExists("html/upload/klasor/altklasor/yeni.txt"))
		return 1;
	req.method = "DELETE";
	req.body = "";
	if (run(srv, req) != 204 || fileExists("html/upload/klasor/altklasor/yeni.txt"))
		return 1;

	// --- POST: var olan dosyanin uzerine yazma ---
	req.method = "POST";
	req.path = "/upload/not.txt";
	req.headers["content-type"] = "text/plain";
	req.body = "yeni icerik";
	if (run(srv, req) != 201 || fileContent("html/upload/klasor/not.txt") != "yeni icerik")
		return 1;
	std::remove("html/upload/klasor/not.txt");
	std::remove("html/upload/klasor/ikili.bin");

	// --- POST: multipart/form-data -> 201, ad Content-Disposition'dan ---
	req.path = "/upload";
	req.headers["content-type"] = "multipart/form-data; boundary=SINIR";
	req.body = "--SINIR\r\nContent-Disposition: form-data; name=\"f\"; filename=\"k.txt\"\r\n\r\nveri\r\n--SINIR--\r\n";
	if (run(srv, req) != 201 || fileContent("html/upload/klasor/k.txt") != "veri")
		return 1;
	std::remove("html/upload/klasor/k.txt");

	// --- POST: multipart, filename'siz field atlanir, ikili icerik korunur ---
	// Null byte'ta kesilirse boyut tutmaz; "name" fonksiyonu filename'a
	// karismamali (karisirsa part atlanir ve 400 doner).
	req.body = "--SINIR\r\nContent-Disposition: form-data; name=\"aciklama\"\r\n\r\nduz metin\r\n";
	req.body += "--SINIR\r\nContent-Disposition: form-data; name=\"f\"; filename=\"ikili.bin\"\r\n\r\n";
	req.body += std::string("AB\0CD", 5);
	req.body += "\r\n--SINIR--\r\n";
	if (run(srv, req) != 201 || fileContent("html/upload/klasor/ikili.bin").size() != 5)
		return 1;
	std::remove("html/upload/klasor/ikili.bin");

	// --- POST: boundary'siz/bozuk multipart -> 400 ---
	req.body = "bu hic boundary icermiyor";
	if (run(srv, req) != 400)
		return 1;

	req.headers["content-type"] = "multipart/form-data; boundary=SINIR";
	req.body = "--SINIR\r\nContent-Disposition: form-data; name=\"f\"; filename=\"yarim.txt\"\r\n\r\nveri";
	if (run(srv, req) != 400)
		return 1;

	// --- POST: filename path traversal -> 400, upload_store disina yazmamali ---
	req.body = "--SINIR\r\nContent-Disposition: form-data; name=\"f\"; filename=\"../evil.txt\"\r\n\r\nEZILDI\r\n--SINIR--\r\n";
	if (run(srv, req) != 400 || fileExists("html/upload/evil.txt"))
		return 1;

	// --- POST: URL dosya adi vermiyor -> 201, adi sunucu uretir ---
	// Location'daki URL'nin gosterdigi dosyada govde olmali; iki POST iki ayri dosya
	req.path = "/upload";
	req.headers["content-type"] = "text/plain";
	req.body = "veri";
	{
		RequestHandler h1(srv);
		HttpResponse r1 = h1.handle(req).response;
		RequestHandler h2(srv);
		HttpResponse r2 = h2.handle(req).response;
		std::string loc1 = r1.headers["Location"];
		std::string loc2 = r2.headers["Location"];
		std::cout << "\n>>> POST /upload (adsiz x2) -> " << r1.status << " " << loc1
			<< " | " << r2.status << " " << loc2 << "\n";
		if (r1.status != 201 || r2.status != 201 || loc1 == loc2
			|| loc1.compare(0, 8, "/upload/") != 0
			|| fileContent("html/upload/klasor/" + loc1.substr(8)) != "veri"
			|| fileContent("html/upload/klasor/" + loc2.substr(8)) != "veri")
			return 1;
		std::remove(("html/upload/klasor/" + loc1.substr(8)).c_str());
		std::remove(("html/upload/klasor/" + loc2.substr(8)).c_str());

		// '/' ile biten alt dizin: ad o dizinin icinde uretilir
		req.path = "/upload/altklasor/";
		RequestHandler h3(srv);
		HttpResponse r3 = h3.handle(req).response;
		std::string loc3 = r3.headers["Location"];
		std::cout << ">>> POST /upload/altklasor/ -> " << r3.status << " " << loc3 << "\n";
		if (r3.status != 201 || loc3.compare(0, 18, "/upload/altklasor/") != 0
			|| fileContent("html/upload/klasor/" + loc3.substr(8)) != "veri")
			return 1;
		std::remove(("html/upload/klasor/" + loc3.substr(8)).c_str());

		// '/' ile biten ama olmayan dizin: 404, "yok" adinda dosya olusmamali
		req.path = "/upload/yok/";
		RequestHandler h4(srv);
		int s4 = h4.handle(req).response.status;
		std::cout << ">>> POST /upload/yok/ -> " << s4 << "\n";
		if (s4 != 404 || fileExists("html/upload/klasor/yok"))
			return 1;
	}

	// --- POST: yukleme kapali location (/ sadece GET) -> 405 ---
	req.path = "/index.html";
	req.body = "veri";
	if (run(srv, req) != 405 || fileContent("html/index.html").empty())
		return 1;

	// --- POST: ".." iceren yol upload_store disina yazmamali ---
	// Yol normalize edilince "/upload/b3_post.txt" olur, yani dosya
	// upload_store altinda olusmali; disarida duran ayni isimli dosyaya
	// dokunulmamali.
	std::ofstream("html/upload/b3_post.txt").put('x');
	req.path = "/upload/../upload/b3_post.txt";
	req.body = "EZILDI";
	if (run(srv, req) != 201
		|| !fileExists("html/upload/klasor/b3_post.txt")
		|| fileContent("html/upload/b3_post.txt") != "x")
		return 1;
	std::remove("html/upload/klasor/b3_post.txt");
	std::remove("html/upload/b3_post.txt");

	req.headers.erase("content-type");
	req.body = "";

	// --- POST: body siniri asiliyor (location siniri 10 byte) -> 413 ---
	// Fixture'in kucuk limitli kopyasi: kalan testleri etkilemesin.
	// bodyLimit() artik handle() icinde calisiyor; govde diske yazilmamali.
	ServerConfig kucuk = srv;
	kucuk.locations[1].client_max_body_size = 10;
	req.method = "POST";
	req.path = "/upload/not.txt";
	req.headers["content-type"] = "text/plain";
	req.body = "bu govde siniri asiyor";
	if (run(kucuk, req) != 413 || fileExists("html/upload/klasor/not.txt"))
		return 1;

//	--- DELETE: var olan dosya -> 204, govdesiz ---
	req.method = "DELETE";
	req.path = "/upload/silinecek.txt";
	req.headers.erase("content-type");
	req.body = "";
	if (run(srv, req) != 204)
		return 1;

	// --- DELETE: ayni istek ikinci kez -> 404 ---
	req.method = "DELETE";
	req.path = "/upload/silinecek.txt";
	req.body = "";
	if (run(srv, req) != 404)
		return 1;

	// --- DELETE: dizin silme desteklenmiyor -> 403 ---
	req.method = "DELETE";
	req.path = "/upload/altklasor/";
	req.body = "";
	int directoryStatus = run(srv, req);
	std::remove("html/upload/klasor/altklasor/korunacak.txt");
	std::remove("html/upload/klasor/altklasor");
	if (directoryStatus != 403)
		return 1;

	// --- DELETE: izin verilmeyen location (/ sadece GET) -> 405 ---
	req.method = "DELETE";
	req.path = "/index.html";
	req.body = "";
	if (run(srv, req) != 405)
		return 1;

	// /upload2, /upload location'ina girmemeli.
	req.path = "/upload2.txt";
	if (run(srv, req) != 405 || !fileExists("html/upload/klasor/2.txt"))
		return 1;
	std::remove("html/upload/klasor/2.txt");

	// Normalize edilen yol upload_store disina cikmamali.
	req.path = "/upload/step/../../upload/b3_escaped.txt";
	if (run(srv, req) != 404 || !fileExists("html/upload/b3_escaped.txt"))
		return 1;
	std::remove("html/upload/b3_escaped.txt");
	std::remove("html/upload/klasor/step");

	// Dis klasore giden sembolik link takip edilmemeli.
	req.path = "/upload/b3_link";
	if (run(srv, req) != 403 || !fileExists("html/upload/b3_outside/korunacak.txt"))
		return 1;
	std::remove("html/upload/klasor/b3_link");
	std::remove("html/upload/b3_outside/korunacak.txt");
	std::remove("html/upload/b3_outside");

	return 0;
}
