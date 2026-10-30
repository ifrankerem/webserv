// B1 router tests. A's code is not needed: the request is filled by hand and
// handle() is called directly.
//   make test

#include <iostream>
#include <string>
#include "RequestHandler.hpp"
#include "Utils.hpp"

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

static void check(bool ok, const char* expr, const char* file, int line)
{
    if (ok) {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::cout << "  FAIL " << file << ":" << line << "  " << expr << "\n";
}

static void section(const char* name)
{
    std::cout << "- " << name << "\n";
}

// --- fixtures ----------------------------------------------------------------

static LocationConfig makeLocation(const std::string& path)
{
    LocationConfig loc;
    loc.path = path;
    loc.root = "www";
    loc.index.push_back("index.html");
    loc.autoindex = false;
    loc.client_max_body_size = 1000;
    loc.redirect_code = 0;
    return loc;
}

// Same shape as conf/default.conf, first server block.
static ServerConfig makeServer()
{
    ServerConfig srv;
    Listen l;
    l.ip = "127.0.0.1";
    l.port = 8080;
    srv.listens.push_back(l);
    srv.root = "www";
    srv.index.push_back("index.html");
    srv.client_max_body_size = 10000;

    LocationConfig root = makeLocation("/");
    root.allowed_methods.push_back("GET");
    root.allowed_methods.push_back("POST");
    srv.locations.push_back(root);

    LocationConfig kapouet = makeLocation("/kapouet");
    kapouet.root = "/tmp";
    kapouet.allowed_methods.push_back("GET");
    srv.locations.push_back(kapouet);

    LocationConfig old = makeLocation("/eski-sayfa");
    old.allowed_methods.push_back("GET");
    old.redirect_code = 301;
    old.redirect_target = "/yeni-sayfa";
    srv.locations.push_back(old);

    LocationConfig upload = makeLocation("/upload");
    upload.allowed_methods.push_back("GET");
    upload.allowed_methods.push_back("POST");
    upload.allowed_methods.push_back("DELETE");
    upload.client_max_body_size = 5;
    upload.upload_store = "www/uploads";
    srv.locations.push_back(upload);

    return srv;
}

static HttpRequest makeRequest(const std::string& method, const std::string& path)
{
    HttpRequest req;
    req.method = method;
    req.path = path;
    req.version = "HTTP/1.1";
    req.headers["host"] = "localhost";
    req.remote_addr = "127.0.0.1";
    req.server_port = 8080;
    return req;
}

static Step run(const ServerConfig& srv, const std::string& method, const std::string& path)
{
    RequestHandler h(srv);
    HttpRequest req = makeRequest(method, path);
    return h.handle(req);
}

static std::string header(const Step& s, const std::string& name)
{
    std::map<std::string, std::string>::const_iterator it = s.response.headers.find(name);
    if (it == s.response.headers.end())
        return "";
    return it->second;
}

// --- tests -------------------------------------------------------------------

static void testBodyLimit(const ServerConfig& srv)
{
    section("bodyLimit: longest prefix location wins");
    RequestHandler h(srv);
    CHECK(h.bodyLimit(makeRequest("POST", "/upload")) == 5);
    CHECK(h.bodyLimit(makeRequest("POST", "/upload/a.txt")) == 5);
    CHECK(h.bodyLimit(makeRequest("POST", "/")) == 1000);
    CHECK(h.bodyLimit(makeRequest("POST", "/baska/yol")) == 1000);
}

static void testMethodNotAllowed(const ServerConfig& srv)
{
    section("405 + Allow");
    Step s = run(srv, "POST", "/kapouet/a.html");
    CHECK(s.kind == Step::RESPOND);
    CHECK(s.response.status == 405);
    CHECK(header(s, "Allow").find("GET") != std::string::npos);
    CHECK(header(s, "Allow").find("POST") == std::string::npos);

    Step d = run(srv, "DELETE", "/index.html");
    CHECK(d.response.status == 405);

    // "/" allows POST, so this must not be 405.
    Step ok = run(srv, "POST", "/baska");
    CHECK(ok.response.status != 405);
}

static void testRedirect(const ServerConfig& srv)
{
    section("redirect");
    Step s = run(srv, "GET", "/eski-sayfa");
    CHECK(s.kind == Step::RESPOND);
    CHECK(s.response.status == 301);
    CHECK(header(s, "Location") == "/yeni-sayfa");
}

static void testPathSafety(const ServerConfig& srv)
{
    section("path safety: never escape root");
    CHECK(run(srv, "GET", "/../etc/passwd").response.status != 200);
    CHECK(run(srv, "GET", "/a/../../etc/passwd").response.status != 200);
    CHECK(run(srv, "GET", "/%2e%2e/etc/passwd").response.status == 400);
    CHECK(run(srv, "GET", "/a/..%2f..%2fetc/passwd").response.status == 400);
    CHECK(run(srv, "GET", "/a%00b").response.status == 400);
    CHECK(run(srv, "GET", "/a/./../../etc/passwd").response.status != 200);
    // "." and ".." that stay inside the root are fine.
    CHECK(run(srv, "GET", "/a/../index.html").response.status != 400);
}

// A splits "/x?a=1" into path "/x" and query "a=1"; routing uses path only.
static void testQueryIgnored(const ServerConfig& srv)
{
    section("query does not affect routing");
    RequestHandler h(srv);

    HttpRequest redirect = makeRequest("GET", "/eski-sayfa");
    redirect.query = "a=1&b=2";
    CHECK(h.handle(redirect).response.status == 301);

    HttpRequest notAllowed = makeRequest("POST", "/kapouet/a.html");
    notAllowed.query = "x=%2Fupload";
    CHECK(h.handle(notAllowed).response.status == 405);

    HttpRequest limit = makeRequest("POST", "/upload/a.txt");
    limit.query = "path=/";
    CHECK(h.bodyLimit(limit) == 5);
}

static void testUtils()
{
    section("utils: path and string helpers");
    CHECK(utils::decodePath("/a%20b") == "/a b");
    CHECK(utils::decodePath("/c+d") == "/c+d");
    CHECK(utils::decodePath("/bad%zz%4") == "/bad%zz%4");
    CHECK(utils::urlDecode("a+b%21") == "a b!");
    CHECK(utils::endsWithSlash("/x/"));
    CHECK(!utils::endsWithSlash("/x"));
    CHECK(!utils::endsWithSlash(""));
    CHECK(utils::joinPath("www/", "/a") == "www/a");
    CHECK(utils::joinPath("www", "a") == "www/a");
    CHECK(utils::joinPath("www/", "") == "www/");
    CHECK(utils::trim("  text/plain \t") == "text/plain");
    CHECK(utils::trim("   ") == "");
    CHECK(utils::atoi("2147483647") == 2147483647);
    CHECK(utils::atoi("2147483648") == -1);
    CHECK(utils::atoi("12a") == -1);
    CHECK(utils::atoi("") == -1);
    CHECK(utils::getExtension("/a.d/b") == "");
    CHECK(utils::getExtension("/a/b.py") == ".py");
}

static void testAlwaysResponds(const ServerConfig& srv)
{
    section("router phase: handle() always returns RESPOND");
    CHECK(run(srv, "GET", "/").kind == Step::RESPOND);
    CHECK(run(srv, "GET", "/yok-boyle-bir-sey").kind == Step::RESPOND);
    CHECK(run(srv, "POST", "/kapouet").kind == Step::RESPOND);
}

int main()
{
    ServerConfig srv = makeServer();

    testBodyLimit(srv);
    testMethodNotAllowed(srv);
    testRedirect(srv);
    testPathSafety(srv);
   testQueryIgnored(srv);
    testAlwaysResponds(srv);
    testUtils();

    std::cout << "\n" << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
