# TODO

---

- [ ] **respond_file()** — serve files from `static/`
  - detect Content-Type from extension (`.html`, `.css`, `.js`, `.png` …)
  - use `sendfile()` on Linux for zero-copy
  - handle `If-Modified-Since` / `304 Not Modified`

- [ ] **Query string parsing** — e.g. `/search?q=hello&page=2`
  - strip `?…` from URI before route matching (already done)
  - add `request_query(req, "q")` helper

- [ ] **Logging middleware** — structured request log
  - `[2024-07-01 12:00:00] GET /users/42 → 200  (1.2ms)`
  - write to stdout or a log file

- [ ] **Custom 404 / 405 pages**
  - call `router.not_found = my_handler` after `router_init()`

- [ ] **Chunked / multi-recv request reading**
  - current code assumes the whole request fits in one `recv()`
  - buffer until `\r\n\r\n` then read `Content-Length` bytes for body

- [ ] **HTTP keep-alive** — reuse the TCP connection for multiple requests
  - check `Connection: keep-alive` header
  - loop `recv → parse → dispatch` on the same fd

- [ ] **Thread pool** — one thread per connection (or a fixed pool)
  - `pthread_create()` per accepted fd, or a pre-spawned pool + queue
  - add mutexes around any shared state (router is read-only, safe)

- [ ] **Wildcard routes** — `/files/*`
  - extend `route_match()` to handle `*` as "rest of path"

- [ ] **Middleware chain** — CORS, auth, rate-limit as composable wrappers
  ```c
  typedef int (*Middleware)(int fd, Request *req, NextFn next);
  ```

- [ ] **URL decode** — `%20` → space, `%2F` → `/` in URI and query params

- [ ] **epoll / kqueue event loop** — replace `select()` for >1k connections
  - Linux: `epoll_create1`, `epoll_ctl`, `epoll_wait`
  - macOS: `kqueue` + `kevent`
  - consider libuv or libev if you want cross-platform

- [ ] **TLS (HTTPS)** — wrap sockets with OpenSSL or mbedTLS
  - `SSL_CTX`, `SSL_accept()`, `SSL_read()`, `SSL_write()`
  - generate a self-signed cert for local dev: `make cert`

- [ ] **HTTP/2** — binary framing, multiplexing, header compression (HPACK)
  - large scope; start with `nghttp2` library

- [ ] **WebSockets** — upgrade HTTP connection
  - parse `Upgrade: websocket` handshake
  - implement frame encode/decode (RFC 6455)

- [ ] **Config file** — port, log level, static root, timeouts from a `.conf`

- [ ] **Unit tests** — test request parser, router matcher independently
  - no framework needed: a `tests/` dir + assertions + `make test-unit`

---

## I should read these

- RFC 7230 — HTTP/1.1 Message Syntax  
  https://datatracker.ietf.org/doc/html/rfc7230

- RFC 7231 — HTTP/1.1 Semantics  
  https://datatracker.ietf.org/doc/html/rfc7231

- Beej's Guide to Network Programming  
  https://beej.us/guide/bgnet/

- The Linux Programming Interface (book) — chapters 56–61 cover sockets in depth

- nginx source (for inspiration on production patterns)  
  https://github.com/nginx/nginx
