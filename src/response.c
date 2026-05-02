/*
 * response.c — HTTP/1.1 response builder + sender
 *
 * Usage pattern:
 *   Response r;
 *   response_init(&r, 200);
 *   response_set_hdr(&r, "Content-Type", "text/plain");
 *   response_set_body(&r, "Hello!", 6, 0);   // 0 = not owned
 *   response_send(&r, client_fd);
 *   response_destroy(&r);
 *
 * Or use the shortcut helpers:
 *   respond_text(fd, 200, "Hello!");
 *   respond_json(fd, 201, "{\"ok\":true}");
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "response.h"

/* ── status lines ─────────────────────────────────── */

const char *status_text(int code)
{
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "Unknown";
    }
}

/* ── lifecycle ────────────────────────────────────── */

void response_init(Response *r, int status_code)
{
    memset(r, 0, sizeof(*r));
    r->status_code = status_code;
    /* Default headers */
    response_set_hdr(r, "Server", "tiny-httpd/0.1");
    response_set_hdr(r, "Connection", "close");
}

void response_set_hdr(Response *r, const char *key, const char *value)
{
    if (r->header_count >= RESP_MAX_HEADERS) return;
    strncpy(r->headers[r->header_count][0], key,   255);
    strncpy(r->headers[r->header_count][1], value, 255);
    r->header_count++;
}

void response_set_body(Response *r, const char *body, int len, int owned)
{
    r->body       = (char *)body;
    r->body_len   = len;
    r->body_owned = owned;
}

int response_send(Response *r, int fd)
{
    /* ── status line ── */
    char head[4096];
    int n = snprintf(head, sizeof(head), "HTTP/1.1 %d %s\r\n",
                     r->status_code, status_text(r->status_code));

    /* ── headers ── */
    for (int i = 0; i < r->header_count; i++) {
        n += snprintf(head + n, sizeof(head) - n,
                      "%s: %s\r\n",
                      r->headers[i][0], r->headers[i][1]);
    }
    /* Content-Length */
    n += snprintf(head + n, sizeof(head) - n,
                  "Content-Length: %d\r\n\r\n", r->body_len);

    send(fd, head, n, 0);

    if (r->body && r->body_len > 0)
        send(fd, r->body, r->body_len, 0);

    return 0;
}

void response_destroy(Response *r)
{
    if (r->body_owned && r->body) {
        free(r->body);
        r->body = NULL;
    }
}

/* ── shortcut helpers ─────────────────────────────── */

void respond_text(int fd, int status, const char *body)
{
    Response r;
    response_init(&r, status);
    response_set_hdr(&r, "Content-Type", "text/plain; charset=utf-8");
    response_set_body(&r, body, (int)strlen(body), 0);
    response_send(&r, fd);
    response_destroy(&r);
}

void respond_json(int fd, int status, const char *json)
{
    Response r;
    response_init(&r, status);
    response_set_hdr(&r, "Content-Type", "application/json");
    response_set_body(&r, json, (int)strlen(json), 0);
    response_send(&r, fd);
    response_destroy(&r);
}

void respond_file(int fd, const char *filepath)
{
    /* TODO: implement static file serving — see TODO.md */
    (void)fd; (void)filepath;
    fprintf(stderr, "[response] respond_file() not yet implemented\n");
}
