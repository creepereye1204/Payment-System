#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "protocol.h"
#include "validator.h"
#include "db.h"
#include "logger.h"

int             g_client_count  = 0;
pthread_mutex_t mutex_client_count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_txn_cache    = PTHREAD_MUTEX_INITIALIZER;
char            txn_cache[TXN_CACHE_SIZE][REF_ID_LEN];
int             txn_cache_count = 0;

typedef struct {
    int    fd;
    char   ip[INET_ADDRSTRLEN];
} ClientCtx;

void *handle_client(void *arg) {
    ClientCtx *ctx = (ClientCtx *)arg;
    int client_fd  = ctx->fd;
    char client_ip[INET_ADDRSTRLEN];
    strncpy(client_ip, ctx->ip, INET_ADDRSTRLEN - 1);
    client_ip[INET_ADDRSTRLEN - 1] = '\0';
    free(ctx);

    pthread_detach(pthread_self());

    /* 소켓 수신 타임아웃 설정 */
    struct timeval timeout;
    timeout.tv_sec  = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    PayRequest  req;
    PayResponse resp;

    ssize_t n = recv(client_fd, &req, sizeof(PayRequest), MSG_WAITALL);
    if (n != (ssize_t)sizeof(PayRequest)) {
        log_error("RECV_FAILED", client_ip);
        goto cleanup;
    }

    /* 엔디안 변환 (네트워크 → 호스트) */
    req.amount    = ntohl(req.amount);
    req.timestamp = ntohl(req.timestamp);

    /* DB 연결 — 스레드마다 독립 연결 */
    MYSQL *conn = db_connect();
    if (!conn) {
        memset(&resp, 0, sizeof(resp));
        resp.status = STATUS_DECLINED;
        strncpy(resp.reason, "DB_UNAVAILABLE", sizeof(resp.reason) - 1);
        send(client_fd, &resp, sizeof(PayResponse), 0);
        goto cleanup;
    }

    int result = validate_request(conn, &req, &resp);
    db_close(conn);

    /* 로그 기록 */
    char masked[20];
    mask_card_no(req.card_no, masked);
    log_transaction(resp.status, resp.txn_id, masked, req.amount, req.merchant_id);

    /* 응답 전송 (approved_at은 validator에서 이미 htonl 처리됨) */
    (void)result;
    send(client_fd, &resp, sizeof(PayResponse), 0);

cleanup:
    close(client_fd);

    pthread_mutex_lock(&mutex_client_count);
    g_client_count--;
    pthread_mutex_unlock(&mutex_client_count);

    return NULL;
}

void run_server(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Listening on port %d (max clients: %d)\n", PORT, MAX_CLIENTS);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            log_error("ACCEPT_FAILED", "accept() error");
            continue;
        }

        pthread_mutex_lock(&mutex_client_count);
        if (g_client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&mutex_client_count);
            close(client_fd);
            log_error("MAX_CLIENTS_REACHED", "connection rejected");
            continue;
        }
        g_client_count++;
        pthread_mutex_unlock(&mutex_client_count);

        ClientCtx *ctx = malloc(sizeof(ClientCtx));
        if (!ctx) {
            close(client_fd);
            pthread_mutex_lock(&mutex_client_count);
            g_client_count--;
            pthread_mutex_unlock(&mutex_client_count);
            log_error("MALLOC_FAILED", "ClientCtx allocation failed");
            continue;
        }
        ctx->fd = client_fd;
        inet_ntop(AF_INET, &client_addr.sin_addr, ctx->ip, INET_ADDRSTRLEN);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, ctx) != 0) {
            log_error("PTHREAD_CREATE_FAILED", "pthread_create() error");
            free(ctx);
            close(client_fd);
            pthread_mutex_lock(&mutex_client_count);
            g_client_count--;
            pthread_mutex_unlock(&mutex_client_count);
        }
    }

    close(server_fd);
}
