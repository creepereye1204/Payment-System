#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include "logger.h"

#define LOG_DIR "log"

static void ensure_log_dir(void) {
    mkdir(LOG_DIR, 0755);
}

static void get_date_log_path(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buf, size, "%s/%04d%02d%02d.log", LOG_DIR,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static void get_timestamp_str(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

void log_transaction(uint8_t status, const char *txn_id,
                     const char *masked_card, uint32_t amount,
                     const char *merchant_id) {
    ensure_log_dir();

    char path[64];
    get_date_log_path(path, sizeof(path));

    FILE *f = fopen(path, "a");
    if (!f) {
        log_error("LOG_OPEN_FAILED", path);
        return;
    }

    char ts[32];
    get_timestamp_str(ts, sizeof(ts));

    const char *status_str = (status == 0x00) ? "APPROVED" : "DECLINED";

    fprintf(f, "[%s] %-8s  %-32s  %-20s  %u  %s\n",
            ts, status_str, txn_id, masked_card, amount, merchant_id);

    fclose(f);
}

void log_error(const char *tag, const char *message) {
    ensure_log_dir();

    FILE *f = fopen(LOG_DIR "/error.log", "a");
    if (!f) return;

    char ts[32];
    get_timestamp_str(ts, sizeof(ts));

    fprintf(f, "[%s] ERROR  %-30s  %s\n", ts, tag, message);

    fclose(f);
}
