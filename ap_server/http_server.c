#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "device_info.h"

#define PORT 80

// 간단한 MIME 지정 (필요 시 확장)
static const char* guess_mime(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "text/plain";
    if (!strcmp(ext, ".html")) return "text/html; charset=UTF-8";
    if (!strcmp(ext, ".json")) return "application/json";
    if (!strcmp(ext, ".css"))  return "text/css";
    if (!strcmp(ext, ".js"))   return "application/javascript";
    return "application/octet-stream";
}

// 정적 파일 전송
static void send_file(int client, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        dprintf(client, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    dprintf(client, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", guess_mime(path));
    char buf[2048];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        write(client, buf, n);
    }
    fclose(fp);
}

// SSID/PASS JSON
static void send_info_json(int client) {
    char ssid[64] = "unknown";
    char pass[64] = "unknown";
    get_ap_info(ssid, sizeof(ssid), pass, sizeof(pass));


    dprintf(client, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
    dprintf(client, "{\"ssid\":\"%s\",\"password\":\"%s\"}", ssid, pass);
}

// 연결 기기 목록 JSON
static void send_devices_json(int client) {
    DeviceInfo devices[64];
    int n = load_connected_devices(devices, 64);

    dprintf(client, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n[");
    for (int i = 0; i < n; i++) {
        dprintf(client,
            "{\"ip\":\"%s\",\"mac\":\"%s\",\"name\":\"%s\"}%s",
            devices[i].ip, devices[i].mac, devices[i].name,
            (i == n - 1) ? "" : ",");
    }
    dprintf(client, "]");
}

// v1.001 추가 버전 ====================================================================
// 상태 JSON (인터넷 연결, CPU 온도 등)
static void send_status_json(int client) {
    FILE *fp;
    char temp[32] = "N/A";
    char up[64] = "N/A";

    // CPU 온도
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp) {
        int t;
        if (fscanf(fp, "%d", &t) == 1)
            snprintf(temp, sizeof(temp), "%.1f°C", t / 1000.0);
        fclose(fp);
    }

    // 업타임
    fp = fopen("/proc/uptime", "r");
    if (fp) {
        double u;
        if (fscanf(fp, "%lf", &u) == 1)
            snprintf(up, sizeof(up), "%.0f sec", u);
        fclose(fp);
    }

    // 인터넷 연결 (ping)
    int online = (system("ping -c1 -W1 8.8.8.8 > /dev/null 2>&1") == 0);

    dprintf(client,
      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
      "{\"cpu_temp\":\"%s\",\"uptime\":\"%s\",\"internet\":%s}",
      temp, up, online ? "true" : "false");
}

static void send_signal_json(int client) {
    FILE *fp = popen("iw dev wlan0 station dump", "r");
    if (!fp) {
        dprintf(client, "HTTP/1.1 500 Internal Error\r\n\r\n");
        return;
    }

    char line[256], mac[32]="", signal[32]="";
    dprintf(client, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n[");

    int first = 1;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Station ")) {
            sscanf(line, "Station %31s", mac);
        } else if (strstr(line, "signal:")) {
            sscanf(line, "signal: %31s", signal);
            if (!first) dprintf(client, ",");
            first = 0;
            dprintf(client, "{\"mac\":\"%s\",\"signal\":\"%sdBm\"}", mac, signal);
        }
    }
    pclose(fp);
    dprintf(client, "]");
}

static void handle_block_mac(int client, const char *req) {
    char mac[64];
    if (sscanf(req, "GET /block?mac=%63[^ ]", mac) == 1) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd),
                 "sudo iptables -I FORWARD -m mac --mac-source %s -j DROP", mac);
        system(cmd);
        dprintf(client, "HTTP/1.1 200 OK\r\n\r\n{\"result\":\"blocked %s\"}", mac);
    } else {
        dprintf(client, "HTTP/1.1 400 Bad Request\r\n\r\n");
    }
}



// ========================================================================================================

void run_http_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 8) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    printf("[HTTP] Server started on port %d\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char req[1024] = {0};
        read(client_fd, req, sizeof(req) - 1);

        if (strstr(req, "GET /info.json") == req) {
            send_info_json(client_fd);
        } else if (strstr(req, "GET /devices.json") == req) {
            send_devices_json(client_fd);
        } else if (strstr(req, "GET /status.json") == req) {
            send_status_json(client_fd);
        } else if (strstr(req, "GET /signal.json") == req) {
            send_signal_json(client_fd);
        } else if (strstr(req, "GET /block?mac=") == req) {
            handle_block_mac(client_fd, req);
        } else {
            send_file(client_fd, "/root/ap_server/www/index.html");
        }

        close(client_fd);
    }
}
