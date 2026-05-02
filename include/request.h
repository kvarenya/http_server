#ifndef REQUEST_H
#define REQUEST_H

/* ─────────────────────────────────────────────
 * request.h  —  HTTP/1.1 request model
 * ───────────────────────────────────────────── */

#define REQ_MAX_HEADERS   32
#define REQ_MAX_URI_LEN   2048
#define REQ_MAX_HDR_LEN   256

typedef enum {
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_HEAD,
    METHOD_OPTIONS,
    METHOD_UNKNOWN
} HttpMethod;

typedef struct {
    char key[REQ_MAX_HDR_LEN];
    char value[REQ_MAX_HDR_LEN];
} Header;

typedef struct {
    HttpMethod method;
    char       uri[REQ_MAX_URI_LEN];
    char       version[16];           /* "HTTP/1.1"          */
    Header     headers[REQ_MAX_HEADERS];
    int        header_count;
    const char *body;                 /* points into raw buf */
    int        body_len;
    /* raw buffer owned by caller */
    char       *raw;
    int         raw_len;
} Request;

/*
 * Parse a raw HTTP request from `buf` (length `len`).
 * Returns 0 on success, -1 on parse error.
 * `req->body` points into `buf` — do not free `buf` before `req`.
 */
int  request_parse  (Request *req, char *buf, int len);
void request_destroy(Request *req);   /* free raw buffer      */

/* Convenience */
const char *request_header(const Request *req, const char *key);
const char *method_str    (HttpMethod m);

#endif /* REQUEST_H */
