CC = gcc
CFLAGS = -std=c11 -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -pthread
TARGET = ui_guard

all: $(TARGET)

$(TARGET): ui_guard.c
	$(CC) $(CFLAGS) ui_guard.c -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
