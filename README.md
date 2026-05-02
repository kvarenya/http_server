# http_server

A minimal HTTP/1.1 server written in C from scratch. Built as a summer project for fun

---

## Layout

```
.
├── include/
│   ├── server.h      — TCP listener interface
│   ├── request.h     — HTTP request model
│   ├── response.h    — HTTP response builder
│   └── router.h      — route table + dispatch
├── src/
│   ├── main.c        — entry point, route registration  ← edit this
│   ├── server.c      — socket + select() event loop
│   ├── request.c     — HTTP/1.1 request parser
│   ├── response.c    — response builder + send
│   └── router.c      — pattern matching + dispatch
├── static/           — static files (for future file serving)
├── Makefile
├── README.md
└── TODO.md
```

---

## Quick start

```bash
# Build
make

# Run (default port 8080)
make run

# Run on a different port
make runport PORT=9090

# Smoke-test in another terminal
make test

# Or manually
curl http://localhost:8080/
curl http://localhost:8080/users/42
curl -X POST http://localhost:8080/echo -d "hello!"
```

---

## Adding a route

Everything lives in `src/main.c`.

1. Write a handler function:

```c
static void handle_greet(int fd, const Request *req, const RouteParams *p)
{
    (void)req;
    const char *name = "World";
    for (int i = 0; i < p->count; i++)
        if (!strcmp(p->params[i].key, "name")) name = p->params[i].value;

    char body[128];
    snprintf(body, sizeof(body), "{\"hello\":\"%s\"}", name);
    respond_json(fd, 200, body);
}
```

2. Register it in `main()`:

```c
router_add(&g_router, METHOD_GET, "/greet/:name", handle_greet);
```

3. Test:

```bash
curl http://localhost:8080/greet/Alice
# {"hello":"Alice"}
```

---

## Architecture

```
client TCP connect
        │
   server_run()          select() loop (server.c)
        │
   accept() + recv()
        │
   request_parse()       parse HTTP/1.1 into Request struct
        │
   router_dispatch()     match URI+method → handler
        │
   handler(fd, req, p)   your code in main.c
        │
   respond_json/text()   build + send HTTP response
        │
   close(fd)
```

---

## Compile flags

| Flag | Meaning |
|------|---------|
| `-Wall -Wextra -Wpedantic` | treat warnings seriously |
| `-std=c11` | modern C |
| `-g` | debug symbols (swap for `-O2` in prod) |

---

## Dependencies

None. POSIX sockets only (`sys/socket.h`, `select()`). Runs on Linux and macOS.
