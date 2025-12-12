# RPRouter v1.100

- 라즈비안 OS에서 사용하던 코드를 OpenWrt OS에서 사용가능하게 포팅한 코드이다.
- 이 코드를 사용하려면 아래를 따라라

## Install
```
opkg update
opkg install iputils-arping

```

## LuCI (기존 웹 서버) 끄기
```
/etc/init.d/uhttpd stop
/etc/init.d/uhttpd disable
```

---

## Directory Structure
```
/home/pi/ap_server/
├── Makefile
├── main.c
├── http_server.c
├── device_info.c
├── device_info.h
└── www/
    └── index.html
```

---

## Build & Run
```

# 1. 가상머신에서 환경변수 설정 -> 껏다키면 다시 재설
export PATH=$PATH:~/openwrt/staging_dir/toolchain-aarch64_cortex-a72_gcc-12.3.0_musl/bin
export TARGET_DIR=$(find ~/openwrt/staging_dir -maxdepth 1 -name "target-aarch64_cortex-a72_musl*" -type d | head -n 1)

# 2. 컴파일 명령어 (http_server 실행파일 생성)
aarch64-openwrt-linux-gcc -o http_server main.c http_server.c device_info.c \
    -I"$TARGET_DIR/usr/include" \
    -L"$TARGET_DIR/usr/lib" \
    -O2 -Wall -pthread

# 3. 실행 파일 라즈베리파이로 전송

# 4. 라우터 접속 후:
mkdir -p /root/ap_server
mv /root/www /root/ap_server/
mv /root/http_server /root/ap_server/
chmod +x /root/ap_server/http_server

# 5. 실행
/root/ap_server/http_server &

서버 종료시
killall http_server

```

---

## 수정한 부분
- 추후 적을 예정

---

## 자동 실행
- 추후 추가 예정

