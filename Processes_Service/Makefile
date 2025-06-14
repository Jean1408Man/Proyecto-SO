TARGET := libprocmon.a
SRC := src
INC := include
OBJ := build

CC := gcc
CFLAGS := -std=c11 -Wall -I$(INC)

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))

all: pm_demo

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

pm_demo: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

clean:
	rm -rf $(OBJ) pm_demo

.PHONY: all clean
