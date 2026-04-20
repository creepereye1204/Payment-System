# 라이브러리 표준화 명세

| 라이브러리 | 헤더 | 용도 | 선택 근거 |
|-----------|------|------|----------|
| pthread | `<pthread.h>` | 멀티스레드, 뮤텍스 | POSIX 표준, Linux 기본 내장 |
| libmysqlclient | `<mysql/mysql.h>` | MySQL DB 연동 | MySQL 공식 C API, 안정성 검증 |
| arpa/inet | `<arpa/inet.h>` | 엔디안 변환 (htonl/ntohl) | 네트워크 바이트 오더 POSIX 표준 |
| sys/socket | `<sys/socket.h>` | TCP 소켓 | POSIX 표준 소켓 API |
| sys/time | `<sys/time.h>` | 소켓 타임아웃 (SO_RCVTIMEO) | POSIX 표준 |
| time | `<time.h>` | 타임스탬프, 날짜별 로그 | C 표준 라이브러리 |
| stdint | `<stdint.h>` | uint8_t, uint32_t 고정 크기 타입 | C99 표준, 바이너리 프로토콜 필수 |
