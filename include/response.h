#ifndef RESPONSE_H
#define RESPONSE_H

/* ─────────────────────────────────────────────
 * response.h  —  HTTP/1.1 response builder
 * ───────────────────────────────────────────── */

#define RESP_MAX_HEADERS 16

typedef struct {
    int  status_code;
    char headers[RESP_MAX_HEADERS][2][256];  /* [i][0]=key, [i][1]=val */
    int  header_count;
    char *body;
    int   body_len;
    int   body_owned;   /* 1 → response_destroy will free body */
} Response;

void response_init    (Response *r, int status_code);
void response_set_hdr (Response *r, const char *key, const char *value);
void response_set_body(Response *r, const char *body, int len, int owned);
int  response_send    (Response *r, int client_fd);
void response_destroy (Response *r);

/* Shortcuts */
void respond_text (int fd, int status, const char *body);
void respond_json (int fd, int status, const char *json);
void respond_file (int fd, const char *filepath);   /* TODO: implement */

/* Status line helpers */
const char *status_text(int code);

#endif /* RESPONSE_H */
