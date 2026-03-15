#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "server.h"
#include "logger.h"

int main(void) {
    srand((unsigned int)time(NULL));
    printf("[mini-pg-gateway] starting...\n");
    run_server();
    return 0;
}
