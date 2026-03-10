#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <mysql/mysql.h>
#include "protocol.h"

int  validate_request(MYSQL *conn, PayRequest *req, PayResponse *resp);
void mask_card_no(const char *src, char *dst);

#endif /* VALIDATOR_H */
