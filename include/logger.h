#ifndef LOGGER_H
#define LOGGER_H

#include "protocol.h"

void log_transaction(uint8_t status, const char *txn_id,
                     const char *masked_card, uint32_t amount,
                     const char *merchant_id);
void log_error(const char *tag, const char *message);

#endif /* LOGGER_H */
