#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>

MYSQL *db_connect(void);
void   db_close(MYSQL *conn);

int db_check_blacklist(MYSQL *conn, const char *card_no);
int db_check_daily_limit(MYSQL *conn, const char *card_no, uint32_t amount);
int db_insert_transaction(MYSQL *conn, const char *txn_id, const char *masked_card,
                          uint32_t amount, const char *merchant_id,
                          uint8_t status, const char *reason);
int db_update_daily_limit(MYSQL *conn, const char *card_no, uint32_t amount);

#endif /* DB_H */
