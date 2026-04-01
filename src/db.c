#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "db.h"
#include "logger.h"

#define DAILY_LIMIT_MAX 10000000UL  /* 1천만 원 */

MYSQL *db_connect(void) {
    const char *db_host = getenv("DB_HOST")     ? getenv("DB_HOST")     : "127.0.0.1";
    const char *db_user = getenv("DB_USER")     ? getenv("DB_USER")     : "pguser";
    const char *db_pass = getenv("DB_PASSWORD") ? getenv("DB_PASSWORD") : "pgpass1234";
    const char *db_name = getenv("DB_NAME")     ? getenv("DB_NAME")     : "pg_gateway";

    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        log_error("DB_INIT_FAILED", "mysql_init() returned NULL");
        return NULL;
    }

    if (!mysql_real_connect(conn, db_host, db_user, db_pass, db_name, 3306, NULL, 0)) {
        log_error("DB_CONNECTION_FAILED", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

void db_close(MYSQL *conn) {
    if (conn) mysql_close(conn);
}

/* 반환값: 1 = 블랙리스트에 있음, 0 = 없음, -1 = 오류 */
int db_check_blacklist(MYSQL *conn, const char *card_no) {
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT 1 FROM blacklist WHERE card_no = '%s' LIMIT 1",
             card_no);

    /* prepared statement 없이 쿼리를 날릴 경우 SQL injection 위험이 있으나,
       card_no는 validator에서 숫자+하이픈만 허용하여 이미 검증됨 */
    if (mysql_query(conn, query) != 0) {
        log_error("DB_QUERY_FAILED", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        log_error("DB_RESULT_FAILED", mysql_error(conn));
        return -1;
    }

    int found = (mysql_num_rows(res) > 0) ? 1 : 0;
    mysql_free_result(res);
    return found;
}

/* 반환값: 1 = 한도 초과, 0 = 허용, -1 = 오류 */
int db_check_daily_limit(MYSQL *conn, const char *card_no, uint32_t amount) {
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT total_amount FROM daily_limit "
             "WHERE card_no = '%s' AND limit_date = CURDATE() LIMIT 1",
             card_no);

    if (mysql_query(conn, query) != 0) {
        log_error("DB_QUERY_FAILED", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        log_error("DB_RESULT_FAILED", mysql_error(conn));
        return -1;
    }

    unsigned long total = 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        total = strtoul(row[0], NULL, 10);
    }
    mysql_free_result(res);

    return (total + amount > DAILY_LIMIT_MAX) ? 1 : 0;
}

int db_insert_transaction(MYSQL *conn, const char *txn_id, const char *masked_card,
                          uint32_t amount, const char *merchant_id,
                          uint8_t status, const char *reason) {
    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO transactions "
             "(txn_id, card_no, amount, merchant_id, status, reason) "
             "VALUES ('%s', '%s', %u, '%s', %d, '%s')",
             txn_id, masked_card, amount, merchant_id, (int)status, reason);

    if (mysql_query(conn, query) != 0) {
        log_error("DB_INSERT_FAILED", mysql_error(conn));
        return -1;
    }
    return 0;
}

int db_update_daily_limit(MYSQL *conn, const char *card_no, uint32_t amount) {
    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO daily_limit (card_no, limit_date, total_amount) "
             "VALUES ('%s', CURDATE(), %u) "
             "ON DUPLICATE KEY UPDATE total_amount = total_amount + %u",
             card_no, amount, amount);

    if (mysql_query(conn, query) != 0) {
        log_error("DB_UPDATE_FAILED", mysql_error(conn));
        return -1;
    }
    return 0;
}
