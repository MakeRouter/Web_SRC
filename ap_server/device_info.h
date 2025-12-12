#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <stddef.h>

#define MAX_DEVICES 64

typedef struct {
    char mac[32];
    char ip[32];
    char name[64];
} DeviceInfo;

/**
 * load_connected_devices(devices, max_count)
 *  - dnsmasq leases 파일을 읽어, 개별 장치에 대해 ping 검사(생존 확인)
 *  - 생존하는 장치만 devices 배열에 채움
 *  - 반환: 실제 추가된 장치 수 (<= max_count)
 *
 * 안전성:
 *  - 내부에서 스레드를 사용해 병렬 ping 수행
 *  - devices 배열 경계 체크 (max_count)
 */
int load_connected_devices(DeviceInfo *devices, int max_count);

/**
 * get_ap_info(ssid, ssid_len, pass, pass_len)
 *  - /etc/hostapd/hostapd.conf 에서 ssid, wpa_passphrase 값을 추출
 *  - ssid_len, pass_len 은 호출자가 제공한 버퍼 길이
 *  - 반환: 0 성공, 음수 실패
 */
int get_ap_info(char *ssid, size_t ssid_len, char *pass, size_t pass_len);

#endif /* DEVICE_INFO_H */
