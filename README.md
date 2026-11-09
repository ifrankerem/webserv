# webserv

An HTTP web server written from scratch in C++98, built as part of the 42 School curriculum.
It serves static sites, handles uploads and deletes, runs CGI scripts, and never blocks:
every socket and every CGI pipe goes through a single `epoll` loop.

## Features

- **Non-blocking I/O** with one `epoll` event loop for listening sockets, clients and CGI pipes
- **nginx-style configuration**: multiple `server` blocks, several ports and hosts, per-`location` rules
- **Methods**: `GET`, `HEAD`, `POST`, `DELETE`
- **Static files** with index files, directory listing (`autoindex`) and correct MIME types
- **File uploads** (request body saved to `upload_store`) and file deletion
- **Chunked transfer encoding** for request bodies
- **CGI** via `fork` + `execve`: Python, PHP (`php-cgi`), Perl and C scripts, chosen by file extension
- **Redirects** (`return 301 /new`), custom error pages and body size limits
- **Timeouts**: idle connections are closed after 30 s, hung CGI scripts are killed and answered with `504`
- **Cookies and sessions** (bonus), shown in the demo site

## Build and run

```sh
make
./webserv conf/default.conf
```

Needs a C++98 compiler and Linux (`epoll`). CGI examples need `python3`, `php-cgi`, `perl` and `cc`.

### Demo site

```sh
./webserv conf/demo.conf
```

Then open <http://127.0.0.1:4242>. The demo includes a request inspector, a calculator,
an online C/C++ compiler running over CGI, a cookie/session page, status-code examples
and a second site on port 4243.

## Configuration

```nginx
server {
    listen 127.0.0.1:4242;
    server_name localhost;
    client_max_body_size 10M;
    error_page 404 /404.html;

    location / {
        root demo;
        index index.html;
        allow_methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php-cgi;
    }

    location /uploads {
        root demo;
        allow_methods GET POST DELETE;
        upload_enable on;
        upload_store demo/uploads;
        autoindex on;
    }

    location /old {
        return 301 /;
    }
}
```

| Directive | Where | Meaning |
|---|---|---|
| `listen` | server | `host:port` to bind |
| `server_name` | server | virtual host name |
| `client_max_body_size` | server, location | max request body (`100`, `10K`, `10M`) |
| `error_page` | server | custom page for a status code |
| `root` / `alias` | location | where files are served from |
| `index` | location | default file for a directory |
| `allow_methods` | location | allowed methods; others get `405` |
| `autoindex` | location | `on` to list directories |
| `upload_enable` / `upload_store` | location | accept uploads and where to save them |
| `cgi` | location | extension and interpreter, e.g. `cgi .py /usr/bin/python3;` |
| `return` | location | redirect, e.g. `return 301 /;` |

More examples are in [`conf/`](conf).

## Tests

```sh
make test                      # unit tests: config, parser, router, static, upload, CGI
python3 tests/eval/e2e.py      # end-to-end tests against a running ./webserv
make 42                        # official 42 tester on conf/tester_42.conf
```

`tests/eval/e2e.py` also accepts `--heavy`, `--siege`, `--valgrind`, `--bonus` and `--nc`.
Each check maps to a rule in [`EVAL_RULES.md`](EVAL_RULES.md).

## Project layout

```
src/core/    config parsing, MIME types, helpers
src/event/   epoll loop (ServerManager) and per-client state
src/http/    request parser, routing, responses, CGI
conf/        example configurations
demo/        demo site with CGI pages
tests/       unit and end-to-end tests
```

## Authors

- Mustafa Emre Yılmaz ([@muyilma](https://github.com/muyilma))
- İrfan Kerem Arslan ([@ifrankerem](https://github.com/ifrankerem))
