#include <stdio.h>
#include "device_info.h"

void run_http_server();

int main() {
    printf("[INIT] Starting AP Web Server...\n");
    run_http_server();
    return 0;
}
