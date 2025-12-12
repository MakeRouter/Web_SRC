# Makefile (권장)
CC := gcc
CFLAGS := -O2 -Wall -Wextra -pthread -DUSE_LIBOPING
LIBS := -loping

SRCS := main.c http_server.c device_info.c
OBJS := $(SRCS:.c=.o)
TARGET := http_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)
