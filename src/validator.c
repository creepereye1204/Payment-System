#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "validator.h"
#include "server.h"
#include "db.h"
#include "logger.h"

#define AMOUNT_MAX 50000000U

static int is_valid_card_format(const char *card_no) {
    if (strlen(card_no) != 19) return 0;

    /* xxxx-xxxx-xxxx-xxxx */
    for (int i = 0; i < 19; i++) {
        if (i == 4 || i == 9 || i == 14) {
            if (card_no[i] != '-') return 0;
        } else {
            if (!isdigit((unsigned char)card_no[i])) return 0;
        }
    }
    return 1;
}

void mask_card_no(const char *src, char *dst) {
    /* 입력: 1234-5678-9012-3456 → 출력: 1234-****-****-3456 */
    strncpy(dst, src, 20);
    dst[19] = '\0';

    if (strlen(src) != 19) return;

    /* 5~8번째 자리 마스킹 */
    dst[5] = '*'; dst[6] = '*'; dst[7] = '*'; dst[8] = '*';
    /* 10~13번째 자리 마스킹 */
    dst[10] = '*'; dst[11] = '*'; dst[12] = '*'; dst[13] = '*';
}

static void generate_txn_id(char *buf, size_t size, uint32_t ts) {
    unsigned int rand_val = (unsigned int)(rand() % 1000000);
    snprintf(buf, size, "TXN-%u-%06u", ts, rand_val);
}

static int cache_lookup(const char *ref_id) {
    for (int i = 0; i < txn_cache_count; i++) {
        if (strncmp(txn_cache[i], ref_id, REF_ID_LEN) == 0) return 1;
    }
    return 0;
}

static void cache_insert(const char *ref_id) {
    if (txn_cache_count >= TXN_CACHE_SIZE) {
        /* 가장 오래된 항목을 밀어내는 ring 방식 — 단순화 위해 덮어씀 */
        memmove(txn_cache[0], txn_cache[1],
                sizeof(txn_cache[0]) * (TXN_CACHE_SIZE - 1));
        txn_cache_count = TXN_CACHE_SIZE - 1;
    }
    strncpy(txn_cache[txn_cache_count], ref_id, REF_ID_LEN - 1);
    txn_cache[txn_cache_count][REF_ID_LEN - 1] = '\0';
    txn_cache_count++;
}

static void set_declined(PayResponse *resp, const char *reason) {
    resp->status = STATUS_DECLINED;
    strncpy(resp->reason, reason, sizeof(resp->reason) - 1);
    resp->reason[sizeof(resp->reason) - 1] = '\0';
    memset(resp->txn_id, 0, sizeof(resp->txn_id));
    resp->approved_at = 0;
}

/* 반환값: 0 = 승인, -1 = 거절 */
int validate_request(MYSQL *conn, PayRequest *req, PayResponse *resp) {
    memset(resp, 0, sizeof(PayResponse));

    /* --- 1. 입력값 검증 --- */
    if (req->msg_type != MSG_TYPE_PAY && req->msg_type != MSG_TYPE_CANCEL) {
        set_declined(resp, "INVALID_MSG_TYPE");
        return -1;
    }

    req->card_no[19] = '\0';
    if (!is_valid_card_format(req->card_no)) {
        set_declined(resp, "INVALID_CARD_FORMAT");
        return -1;
    }

    if (req->amount == 0 || req->amount > AMOUNT_MAX) {
        set_declined(resp, "INVALID_AMOUNT");
        return -1;
    }

    req->merchant_id[19] = '\0';
    size_t mid_len = strlen(req->merchant_id);
    if (mid_len == 0 || mid_len > 19) {
        set_declined(resp, "INVALID_MERCHANT_ID");
        return -1;
    }

    req->client_ref_id[31] = '\0';

    /* --- 2. 블랙리스트 조회 --- */
    int bl = db_check_blacklist(conn, req->card_no);
    if (bl == 1) {
        set_declined(resp, "CARD_BLACKLISTED");
        return -1;
    } else if (bl == -1) {
        set_declined(resp, "DB_ERROR");
        return -1;
    }

    /* --- 3. 일일 한도 조회 --- */
    int lim = db_check_daily_limit(conn, req->card_no, req->amount);
    if (lim == 1) {
        set_declined(resp, "DAILY_LIMIT_EXCEEDED");
        return -1;
    } else if (lim == -1) {
        set_declined(resp, "DB_ERROR");
        return -1;
    }

    /* --- 4. 중복 거래 방지 (mutex 보호) --- */
    pthread_mutex_lock(&mutex_txn_cache);
    int dup = cache_lookup(req->client_ref_id);
    if (dup) {
        pthread_mutex_unlock(&mutex_txn_cache);
        set_declined(resp, "DUPLICATE_TRANSACTION");
        return -1;
    }
    pthread_mutex_unlock(&mutex_txn_cache);

    /* --- 5. 승인 처리 --- */
    uint32_t now_ts = (uint32_t)time(NULL);
    char txn_id[32];
    generate_txn_id(txn_id, sizeof(txn_id), now_ts);

    char masked[20];
    mask_card_no(req->card_no, masked);

    /* DB INSERT */
    if (db_insert_transaction(conn, txn_id, masked, req->amount,
                              req->merchant_id, STATUS_APPROVED, "SUCCESS") != 0) {
        set_declined(resp, "DB_ERROR");
        return -1;
    }

    /* 일일 누적 업데이트 */
    db_update_daily_limit(conn, req->card_no, req->amount);

    /* 캐시에 ref_id 추가 */
    pthread_mutex_lock(&mutex_txn_cache);
    cache_insert(req->client_ref_id);
    pthread_mutex_unlock(&mutex_txn_cache);

    /* 응답 빌드 */
    resp->status = STATUS_APPROVED;
    strncpy(resp->txn_id, txn_id, sizeof(resp->txn_id) - 1);
    resp->txn_id[sizeof(resp->txn_id) - 1] = '\0';
    strncpy(resp->reason, "SUCCESS", sizeof(resp->reason) - 1);
    resp->reason[sizeof(resp->reason) - 1] = '\0';
    resp->approved_at = htonl(now_ts);

    return 0;
}
