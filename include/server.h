#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#define PORT         8080
#define MAX_CLIENTS  100
#define BACKLOG      10
#define TIMEOUT_SEC  5

#define TXN_CACHE_SIZE 1000
#define TXN_ID_LEN     32
#define REF_ID_LEN     32

extern int              g_client_count;
extern pthread_mutex_t  mutex_client_count;
extern pthread_mutex_t  mutex_txn_cache;
extern char             txn_cache[TXN_CACHE_SIZE][REF_ID_LEN];
extern int              txn_cache_count;

void *handle_client(void *arg);
void  run_server(void);

#endif /* SERVER_H */
