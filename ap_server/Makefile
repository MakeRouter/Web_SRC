# --------------------------------------------------------
# OpenWrt Cross-Compile Makefile
# --------------------------------------------------------

# 1. 경로 자동 탐색 (사용자의 홈 디렉토리 ~ 기준)
OPENWRT_ROOT := $(HOME)/openwrt
STAGING_DIR  := $(OPENWRT_ROOT)/staging_dir

# TARGET_DIR을 자동으로 찾아서 변수에 저장
TARGET_DIR := $(shell find $(STAGING_DIR) -maxdepth 1 -name "target-aarch64_cortex-a72_musl*" -type d | head -n 1)

# 2. 컴파일러 및 환경변수 설정
export STAGING_DIR
CC := aarch64-openwrt-linux-gcc

# 3. 플래그 설정
CFLAGS  := -O2 -Wall -Wextra -pthread
CFLAGS  += -I"$(TARGET_DIR)/usr/include"

# [수정] 라이브러리 링크(-loping) 제거
LDFLAGS := -L"$(TARGET_DIR)/usr/lib"
LIBS    := 

# 4. 소스 및 타겟
SRCS   := main.c http_server.c device_info.c
TARGET := http_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "[INFO] Compiling for OpenWrt..."
	@echo "       Target Dir: $(TARGET_DIR)"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LIBS)
	@echo "[DONE] Created executable: $@"

clean:
	rm -f $(TARGET)
