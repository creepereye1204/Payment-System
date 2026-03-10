#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#pragma pack(1)

typedef struct {
    uint8_t  msg_type;          /* 0x01 = PAY, 0x02 = CANCEL */
    char     card_no[20];       /* "1234-5678-9012-3456\0" */
    uint32_t amount;            /* 결제 금액 (원), 네트워크 바이트 오더 */
    char     merchant_id[20];   /* 가맹점 ID */
    char     client_ref_id[32]; /* 클라이언트 참조 ID (중복 방지용) */
    uint32_t timestamp;         /* Unix timestamp, 네트워크 바이트 오더 */
} PayRequest;

typedef struct {
    uint8_t  status;            /* 0x00 = APPROVED, 0x01 = DECLINED */
    char     txn_id[32];        /* 서버 생성 거래 고유 ID */
    char     reason[64];        /* 거절 사유 or "SUCCESS" */
    uint32_t approved_at;       /* 승인 시각 Unix timestamp, 네트워크 바이트 오더 */
} PayResponse;

#pragma pack()

#define MSG_TYPE_PAY    0x01
#define MSG_TYPE_CANCEL 0x02

#define STATUS_APPROVED 0x00
#define STATUS_DECLINED 0x01

#endif /* PROTOCOL_H */
