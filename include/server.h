#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

/* ─────────────────────────────────────────────
 * server.h  —  TCP listener / connection loop
 * ───────────────────────────────────────────── */

#define SERVER_DEFAULT_PORT     8080
#define SERVER_DEFAULT_BACKLOG  128
#define SERVER_RECV_BUFSIZE     8192   /* bytes per read() call   */
#define SERVER_MAX_CONNECTIONS  1024   /* fd_set ceiling (select) */

typedef struct {
    int      fd;           /* listening socket fd      */
    uint16_t port;
    int      backlog;
} Server;

/* Lifecycle */
int  server_init   (Server *s, uint16_t port, int backlog);
void server_run    (Server *s);   /* blocking event loop      */
void server_destroy(Server *s);

#endif /* SERVER_H */
