/*
 * main.c — entry point & route registration
 *
 * This is the only file you should need to edit day-to-day.
 * Add routes here, implement handlers below (or in separate files).
 *
 * Build & run:
 *   make
 *   ./build/httpd
 *
 *   # test it
 *   curl http://localhost:8080/
 *   curl http://localhost:8080/users/42
 *   curl -X POST http://localhost:8080/echo -d "hello world"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "server.h"
#include "router.h"
#include "request.h"
#include "response.h"

/* ── Global router (referenced by server.c) ────────── */
Router g_router;
static Server g_server;

/* ─────────────────────────────────────────────────────
 * Route handlers
 * ───────────────────────────────────────────────────── */

static void handle_index(int fd, const Request *req, const RouteParams *p)
{
    (void)req; (void)p;
    respond_json(fd, 200,
        "{"
        "\"name\":\"tiny-httpd\","
        "\"version\":\"0.1\","
        "\"status\":\"ok\""
        "}");
}

static void handle_get_user(int fd, const Request *req, const RouteParams *p)
{
    (void)req;
    /* Extract :id from path params */
    const char *id = NULL;
    for (int i = 0; i < p->count; i++) {
        if (strcmp(p->params[i].key, "id") == 0) {
            id = p->params[i].value;
            break;
        }
    }

    char body[512];
    snprintf(body, sizeof(body),
             "{\"id\":\"%s\",\"name\":\"Alice\"}", id ? id : "?");
    respond_json(fd, 200, body);
}

static void handle_create_user(int fd, const Request *req, const RouteParams *p)
{
    (void)p;
    /* req->body contains the raw request body */
    printf("[handler] POST /users  body=%.*s\n",
           req->body_len, req->body ? req->body : "");
    respond_json(fd, 201, "{\"created\":true}");
}

static void handle_echo(int fd, const Request *req, const RouteParams *p)
{
    (void)p;
    /* Echo back the request body as plain text */
    if (req->body && req->body_len > 0) {
        respond_text(fd, 200, req->body);
    } else {
        respond_text(fd, 200, "(empty body)");
    }
}

static void handle_health(int fd, const Request *req, const RouteParams *p)
{
    (void)req; (void)p;
    respond_json(fd, 200, "{\"healthy\":true}");
}

/* ── graceful shutdown ────────────────────────────── */
static void on_signal(int sig)
{
    (void)sig;
    printf("\n[main] shutting down\n");
    server_destroy(&g_server);
    exit(0);
}

/* ─────────────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    uint16_t port = SERVER_DEFAULT_PORT;
    if (argc == 2) port = (uint16_t)atoi(argv[1]);

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    /* ── Register routes ── */
    router_init(&g_router);
    router_add(&g_router, METHOD_GET,  "/",           handle_index);
    router_add(&g_router, METHOD_GET,  "/health",     handle_health);
    router_add(&g_router, METHOD_GET,  "/users/:id",  handle_get_user);
    router_add(&g_router, METHOD_POST, "/users",      handle_create_user);
    router_add(&g_router, METHOD_POST, "/echo",       handle_echo);
    /* Add your routes here ↓ */

    /* ── Start server ── */
    if (server_init(&g_server, port, SERVER_DEFAULT_BACKLOG) != 0) {
        fprintf(stderr, "[main] failed to start server\n");
        return 1;
    }

    server_run(&g_server);   /* blocks */
    return 0;
}
