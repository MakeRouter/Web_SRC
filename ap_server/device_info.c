#include "device_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef USE_LIBOPING
#include <oping.h>
#endif

#define LEASE_FILE   "/var/dhcp.leases"
#define HOSTAPD_CONF "/var/run/hostapd-phy0.conf"

static pthread_mutex_t dev_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char mac[32];
    char ip[32];
    char name[64];
    DeviceInfo *devices;
    int *count;
    int max_count;
} ThreadArg;

static int alive_ping(const char *ip) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "ping -c 1 -W 1 %s > /dev/null 2>&1", ip);
    return (system(cmd) == 0);
}

/* arp fallback test (arping) */
static int alive_arp(const char *ip) {
    char cmd[160];
    snprintf(cmd, sizeof(cmd),
             "arping -I br-lan -c 1 -w 1 %s > /dev/null 2>&1", ip);
    return (system(cmd) == 0);
}

int is_alive(const char *ip) {
    if (!ip || ip[0] == '\0') return 0;

    /* 1) Ping 먼저 시도 */
    if (alive_ping(ip)) return 1;

    /* 2) Ping 실패 시 → ARP 확인 */
    if (alive_arp(ip)) return 1;

    return 0;
}


/* check_alive_thread: 스레드에서 ping 후 성공하면 devices[]에 추가 */
static void *check_alive_thread(void *arg) {
    ThreadArg *data = (ThreadArg*)arg;
    if (!data) return NULL;

    if (is_alive(data->ip)) {
        pthread_mutex_lock(&dev_mutex);
        int idx = *(data->count);
        if (idx < data->max_count) {
            /* 안전 복사: snprintf 로 널종료 보장 */
            snprintf(data->devices[idx].mac, sizeof(data->devices[idx].mac), "%s", data->mac);
            snprintf(data->devices[idx].ip,  sizeof(data->devices[idx].ip),  "%s", data->ip);
            snprintf(data->devices[idx].name,sizeof(data->devices[idx].name),"%s", data->name);
            (*(data->count))++;
        }
        pthread_mutex_unlock(&dev_mutex);
    }

    free(data);
    return NULL;
}

/* load_connected_devices: leases 파일을 읽고 병렬 ping 수행 */
int load_connected_devices(DeviceInfo *devices, int max_count) {
    if (!devices || max_count <= 0) return 0;

    FILE *fp = fopen(LEASE_FILE, "r");
    if (!fp) return 0;

    char line[512];
    pthread_t threads[MAX_DEVICES];
    int thread_count = 0;
    int count = 0;

    /* leases 파일 형식: <timestamp> <mac> <ip> <hostname> <...> */
    while (fgets(line, sizeof(line), fp) && thread_count < MAX_DEVICES) {
        char ts[32], mac[32], ip[32], hostname[64];

        /* hostname은 첫 공백까지만 */
        if (sscanf(line, "%31s %31s %31s %63s", ts, mac, ip, hostname) != 4) {
            continue;
        }

        ThreadArg *arg = calloc(1, sizeof(ThreadArg));
        if (!arg) continue;

        snprintf(arg->mac, sizeof(arg->mac), "%s", mac);
        snprintf(arg->ip, sizeof(arg->ip), "%s", ip);
        snprintf(arg->name, sizeof(arg->name), "%s", hostname);

        arg->devices = devices;
        arg->count = &count;
        arg->max_count = max_count;

        int rc = pthread_create(&threads[thread_count], NULL, check_alive_thread, arg);
        if (rc != 0) {
            free(arg);
            continue;
        }
        thread_count++;
    }
    fclose(fp);

    /* join all threads */
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    if (count > max_count) count = max_count;
    return count;
}

/* get_ap_info: hostapd.conf에서 ssid/wpa_passphrase 추출 */
int get_ap_info(char *ssid, size_t ssid_len, char *pass, size_t pass_len) {
    if (!ssid || !pass || ssid_len == 0 || pass_len == 0) return -1;

    FILE *fp = fopen(HOSTAPD_CONF, "r");
    if (!fp) return -1;

    char line[512];
    ssid[0] = '\0';
    pass[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "ssid=", 5) == 0) {
            char *val = p + 5;
            val[strcspn(val, "\r\n")] = '\0';
            snprintf(ssid, ssid_len, "%s", val);
        } else if (strncmp(p, "wpa_passphrase=", 15) == 0) {
            char *val = p + 15;
            val[strcspn(val, "\r\n")] = '\0';
            snprintf(pass, pass_len, "%s", val);
        }
    }
    fclose(fp);
    return 0;
}
