# Makefile para Processes_Service

SRC_DIR=src
INCLUDE_DIR=include
CC=gcc
CFLAGS=-I$(INCLUDE_DIR) -Wall -Wextra -g
SRCS=$(SRC_DIR)/main.c $(SRC_DIR)/process_monitor.c
OBJS=$(SRCS:.c=.o)
TARGET=process_monitor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
