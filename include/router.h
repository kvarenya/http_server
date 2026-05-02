#ifndef ROUTER_H
#define ROUTER_H

#include "request.h"
#include "response.h"

/* ─────────────────────────────────────────────
 * router.h  —  radix-like route table
 *
 * Register handlers with router_add(), then call
 * router_dispatch() from the connection handler.
 * ───────────────────────────────────────────── */

#define ROUTER_MAX_ROUTES  64
#define ROUTER_MAX_PARAMS   8   /* path params, e.g. /users/:id */

typedef struct {
    char key[64];
    char value[256];
} PathParam;

typedef struct {
    PathParam params[ROUTER_MAX_PARAMS];
    int       count;
} RouteParams;

/*
 * Handler signature.
 * `params` holds extracted path parameters (e.g. :id).
 */
typedef void (*RouteHandler)(int client_fd,
                             const Request *req,
                             const RouteParams *params);

typedef struct {
    HttpMethod   method;
    char         pattern[256];   /* e.g. "/users/:id"   */
    RouteHandler handler;
} Route;

typedef struct {
    Route routes[ROUTER_MAX_ROUTES];
    int   count;
    /* 404 / 405 fallbacks */
    RouteHandler not_found;
    RouteHandler method_not_allowed;
} Router;

void router_init    (Router *r);
void router_add     (Router *r, HttpMethod method,
                     const char *pattern, RouteHandler handler);
void router_dispatch(Router *r, int client_fd, const Request *req);

/* Default fallback handlers (used if you don't override) */
void default_not_found         (int fd, const Request *req, const RouteParams *p);
void default_method_not_allowed(int fd, const Request *req, const RouteParams *p);

/* Internal: match URI against pattern, fill params */
int route_match(const char *pattern, const char *uri, RouteParams *params);

#endif /* ROUTER_H */
