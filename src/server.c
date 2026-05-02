/*
 * server.c — TCP listener + select()-based connection loop
 *
 * Architecture
 * ────────────
 *  server_init()  →  creates & binds the listening socket
 *  server_run()   →  select() loop; accepts new connections,
 *                    reads complete HTTP requests, dispatches
 *                    to the global router, closes connection.
 *
 * Limitations (see TODO.md for upgrade paths)
 * ───────────────────────────────────────────
 *  • Single-threaded (select, not epoll/kqueue)
 *  • No keep-alive / pipelining
 *  • Full request must arrive in one recv() call
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "request.h"
#include "router.h"

/* Declared in main.c — the application's global router */
extern Router g_router;

/* ── helpers ──────────────────────────────────────── */

static int make_listen_socket(uint16_t port, int backlog)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, backlog) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

static void handle_client(int client_fd)
{
    char buf[SERVER_RECV_BUFSIZE];
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    Request req = {0};
    if (request_parse(&req, buf, n) != 0) {
        /* Malformed request — send 400 */
        const char *bad = "HTTP/1.1 400 Bad Request\r\n"
                          "Content-Length: 11\r\n\r\nBad Request";
        send(client_fd, bad, strlen(bad), 0);
        return;
    }

    router_dispatch(&g_router, client_fd, &req);
    request_destroy(&req);
}

/* ── public API ───────────────────────────────────── */

int server_init(Server *s, uint16_t port, int backlog)
{
    s->port    = port;
    s->backlog = backlog;
    s->fd      = make_listen_socket(port, backlog);
    return s->fd < 0 ? -1 : 0;
}

void server_run(Server *s)
{
    printf("[server] listening on http://localhost:%d\n", s->port);

    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_SET(s->fd, &master);
    int fdmax = s->fd;

    while (1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        for (int fd = 0; fd <= fdmax; fd++) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            if (fd == s->fd) {
                /* New connection */
                struct sockaddr_in ca; socklen_t cal = sizeof(ca);
                int cfd = accept(s->fd, (struct sockaddr *)&ca, &cal);
                if (cfd < 0) { perror("accept"); continue; }
                if (cfd < SERVER_MAX_CONNECTIONS) {
                    FD_SET(cfd, &master);
                    if (cfd > fdmax) fdmax = cfd;
                    printf("[server] +connect  fd=%d  %s\n",
                           cfd, inet_ntoa(ca.sin_addr));
                } else {
                    fprintf(stderr, "[server] fd limit reached, dropping\n");
                    close(cfd);
                }
            } else {
                /* Existing connection — read & handle */
                handle_client(fd);
                FD_CLR(fd, &master);
                close(fd);
                printf("[server] -disconnect fd=%d\n", fd);
            }
        }
    }
}

void server_destroy(Server *s)
{
    close(s->fd);
    s->fd = -1;
}
