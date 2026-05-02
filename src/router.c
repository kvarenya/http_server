/*
 * router.c — Simple route table with :param extraction
 *
 * Patterns
 * ────────
 *   /users            exact match
 *   /users/:id        captures "id" from any segment
 *   /files/STAR       wildcard (matches rest of path)  [TODO]
 *
 * Dispatch order: first registered match wins.
 * If the URI matches but the method doesn't, 405 is returned.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "router.h"
#include "response.h"

/* ── default fallbacks ────────────────────────────── */

void default_not_found(int fd, const Request *req, const RouteParams *p)
{
    (void)req; (void)p;
    respond_json(fd, 404, "{\"error\":\"Not Found\"}");
}

void default_method_not_allowed(int fd, const Request *req, const RouteParams *p)
{
    (void)req; (void)p;
    respond_json(fd, 405, "{\"error\":\"Method Not Allowed\"}");
}

/* ── lifecycle ────────────────────────────────────── */

void router_init(Router *r)
{
    memset(r, 0, sizeof(*r));
    r->not_found          = default_not_found;
    r->method_not_allowed = default_method_not_allowed;
}

void router_add(Router *r, HttpMethod method,
                const char *pattern, RouteHandler handler)
{
    if (r->count >= ROUTER_MAX_ROUTES) {
        fprintf(stderr, "[router] route table full!\n");
        return;
    }
    Route *rt    = &r->routes[r->count++];
    rt->method   = method;
    strncpy(rt->pattern, pattern, sizeof(rt->pattern) - 1);
    rt->handler  = handler;
    printf("[router] registered %s %s\n",
           method_str(method), pattern);
}

/* ── pattern matching ─────────────────────────────── */

/*
 * Split a path into segments on '/'.
 * Returns segment count.  seg[] points into (mutated) buf.
 */
static int split_path(char *buf, char *seg[], int max)
{
    int n = 0;
    char *tok = strtok(buf, "/");
    while (tok && n < max) { seg[n++] = tok; tok = strtok(NULL, "/"); }
    return n;
}

int route_match(const char *pattern, const char *uri, RouteParams *params)
{
    char pbuf[256], ubuf[REQ_MAX_URI_LEN];
    strncpy(pbuf, pattern, sizeof(pbuf) - 1);
    strncpy(ubuf, uri,     sizeof(ubuf) - 1);

    /* Strip query string from uri */
    char *q = strchr(ubuf, '?');
    if (q) *q = '\0';

    char *pseg[32], *useg[32];
    int pc = split_path(pbuf, pseg, 32);
    int uc = split_path(ubuf, useg, 32);

    if (pc != uc) return 0;

    if (params) { memset(params, 0, sizeof(*params)); }

    for (int i = 0; i < pc; i++) {
        if (pseg[i][0] == ':') {
            /* path param */
            if (params && params->count < ROUTER_MAX_PARAMS) {
                PathParam *pp = &params->params[params->count++];
                strncpy(pp->key,   pseg[i] + 1, sizeof(pp->key)   - 1);
                strncpy(pp->value, useg[i],      sizeof(pp->value) - 1);
            }
        } else if (strcmp(pseg[i], useg[i]) != 0) {
            return 0;
        }
    }
    return 1;
}

/* ── dispatch ─────────────────────────────────────── */

void router_dispatch(Router *r, int client_fd, const Request *req)
{
    int uri_matched = 0;
    RouteParams params;

    for (int i = 0; i < r->count; i++) {
        Route *rt = &r->routes[i];
        if (!route_match(rt->pattern, req->uri, &params)) continue;
        uri_matched = 1;

        if (rt->method != req->method) continue;

        rt->handler(client_fd, req, &params);
        return;
    }

    if (uri_matched)
        r->method_not_allowed(client_fd, req, &params);
    else
        r->not_found(client_fd, req, &params);
}
