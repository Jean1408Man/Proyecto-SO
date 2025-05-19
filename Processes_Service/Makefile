# ─────────── Configuración ───────────
TARGET      := libprocmon.a
SRC_DIR     := src
INC_DIR     := include
OBJ_DIR     := build

CC          := gcc
CFLAGS      := -std=c11 -Wall -Wextra -O2 -I$(INC_DIR)
AR          := ar
ARFLAGS     := rcs

SOURCES     := $(wildcard $(SRC_DIR)/*.c)
OBJECTS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS        := $(OBJECTS:.o=.d)

.PHONY: all demo clean

# ─────────── Targets ───────────
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(DEPS)


pm_demo: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

.PHONY: demo
demo: pm_demo


clean:
	rm -rf $(OBJ_DIR) $(TARGET) pm_demo
