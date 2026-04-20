#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdint.h>

#pragma pack(1)
typedef struct {
    uint8_t  msg_type;
    char     card_no[20];
    uint32_t amount;
    char     merchant_id[20];
    char     client_ref_id[32];
    uint32_t timestamp;
} PayRequest;

typedef struct {
    uint8_t  status;
    char     txn_id[32];
    char     reason[64];
    uint32_t approved_at;
} PayResponse;
#pragma pack()

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 18080

static void send_payment(const char *card_no, uint32_t amount,
                         const char *merchant_id, const char *ref_id) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return;
    }

    PayRequest req;
    memset(&req, 0, sizeof(req));
    req.msg_type = 0x01;
    strncpy(req.card_no, card_no, 19);
    req.card_no[19] = '\0';
    req.amount    = htonl(amount);
    strncpy(req.merchant_id, merchant_id, 19);
    req.merchant_id[19] = '\0';
    strncpy(req.client_ref_id, ref_id, 31);
    req.client_ref_id[31] = '\0';
    req.timestamp = htonl((uint32_t)time(NULL));

    ssize_t sent = send(fd, &req, sizeof(PayRequest), 0);
    if (sent != sizeof(PayRequest)) {
        fprintf(stderr, "send failed\n");
        close(fd);
        return;
    }

    PayResponse resp;
    ssize_t n = recv(fd, &resp, sizeof(PayResponse), MSG_WAITALL);
    if (n != sizeof(PayResponse)) {
        fprintf(stderr, "recv failed\n");
        close(fd);
        return;
    }

    uint32_t approved_at = ntohl(resp.approved_at);
    printf("--- 결제 결과 ---\n");
    printf("카드번호    : %s\n", card_no);
    printf("금액        : %u 원\n", amount);
    printf("가맹점      : %s\n", merchant_id);
    printf("상태        : %s\n", resp.status == 0 ? "APPROVED" : "DECLINED");
    printf("거래ID      : %s\n", resp.txn_id[0] ? resp.txn_id : "(없음)");
    printf("사유        : %s\n", resp.reason);
    printf("승인시각    : %u\n", approved_at);
    printf("----------------\n\n");

    close(fd);
}

int main(void) {
    printf("=== mini-pg-gateway 테스트 클라이언트 ===\n\n");

    /* 정상 결제 */
    send_payment("1234-5678-9012-3456", 50000, "MERCHANT_001", "REF-001");

    /* 블랙리스트 카드 */
    send_payment("1111-1111-1111-1111", 10000, "MERCHANT_001", "REF-002");

    /* 금액 초과 */
    send_payment("1234-5678-9012-3456", 60000000, "MERCHANT_001", "REF-003");

    /* 중복 거래 */
    send_payment("1234-5678-9012-3456", 30000, "MERCHANT_001", "REF-001");

    /* 잘못된 카드 형식 */
    send_payment("0000-0000-0000-000X", 1000, "MERCHANT_001", "REF-004");

    return 0;
}
