/*
 * request.c — HTTP/1.1 request parser
 *
 * Parses the request line and headers from a raw buffer.
 * The body pointer aliases into the caller's buffer (zero-copy).
 *
 * Grammar handled:
 *   Request-Line  = Method SP Request-URI SP HTTP-Version CRLF
 *   Header        = field-name ":" OWS field-value CRLF
 *   CRLF          = \r\n
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>   /* strcasecmp */

#include "request.h"

/* ── method parsing ───────────────────────────────── */

static HttpMethod parse_method(const char *s)
{
    if (!strcmp(s, "GET"))     return METHOD_GET;
    if (!strcmp(s, "POST"))    return METHOD_POST;
    if (!strcmp(s, "PUT"))     return METHOD_PUT;
    if (!strcmp(s, "DELETE"))  return METHOD_DELETE;
    if (!strcmp(s, "HEAD"))    return METHOD_HEAD;
    if (!strcmp(s, "OPTIONS")) return METHOD_OPTIONS;
    return METHOD_UNKNOWN;
}

const char *method_str(HttpMethod m)
{
    switch (m) {
        case METHOD_GET:     return "GET";
        case METHOD_POST:    return "POST";
        case METHOD_PUT:     return "PUT";
        case METHOD_DELETE:  return "DELETE";
        case METHOD_HEAD:    return "HEAD";
        case METHOD_OPTIONS: return "OPTIONS";
        default:             return "UNKNOWN";
    }
}

/* ── trim leading OWS (optional whitespace) ───────── */

static char *ltrim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static void rtrim(char *s)
{
    int l = strlen(s);
    while (l > 0 && (s[l-1] == ' ' || s[l-1] == '\t' ||
                     s[l-1] == '\r' || s[l-1] == '\n'))
        s[--l] = '\0';
}

/* ── public API ───────────────────────────────────── */

int request_parse(Request *req, char *buf, int len)
{
    memset(req, 0, sizeof(*req));
    req->raw     = buf;
    req->raw_len = len;

    /* ── Request line ── */
    char *line_end = strstr(buf, "\r\n");
    if (!line_end) return -1;
    *line_end = '\0';

    char method_s[16] = {0};
    if (sscanf(buf, "%15s %2047s %15s",
               method_s, req->uri, req->version) != 3)
        return -1;

    req->method = parse_method(method_s);

    /* ── Headers ── */
    char *cursor = line_end + 2;   /* skip \r\n */
    while (req->header_count < REQ_MAX_HEADERS) {
        char *end = strstr(cursor, "\r\n");
        if (!end) break;
        *end = '\0';

        if (*cursor == '\0') {
            /* blank line → end of headers */
            cursor = end + 2;
            break;
        }

        char *colon = strchr(cursor, ':');
        if (colon) {
            *colon = '\0';
            Header *h = &req->headers[req->header_count++];
            strncpy(h->key,   cursor,          sizeof(h->key)   - 1);
            strncpy(h->value, ltrim(colon + 1), sizeof(h->value) - 1);
            rtrim(h->key);
            rtrim(h->value);
        }
        cursor = end + 2;
    }

    /* ── Body (points into original buf) ── */
    req->body = cursor;
    req->body_len = (buf + len) - cursor;
    if (req->body_len < 0) req->body_len = 0;

    printf("[request] %s %s %s\n",
           method_str(req->method), req->uri, req->version);
    return 0;
}

void request_destroy(Request *req)
{
    /* buf is owned by the caller (server.c stack), nothing to free */
    (void)req;
}

const char *request_header(const Request *req, const char *key)
{
    for (int i = 0; i < req->header_count; i++) {
        /* case-insensitive compare per RFC 7230 */
        if (strcasecmp(req->headers[i].key, key) == 0)
            return req->headers[i].value;
    }
    return NULL;
}
