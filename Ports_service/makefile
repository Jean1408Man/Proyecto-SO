CC        = gcc
CFLAGS    = -Wall -g `pkg-config --cflags glib-2.0` -I$(INC_DIR)
LDFLAGS   = `pkg-config --libs glib-2.0` -pthread

SRC_DIR    = src
INC_DIR    = include
BUILD_DIR  = build
BIN_DIR    = bin
TEST_DIR   = tests

# Código fuente del escáner
SRCS       = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/port_scanner.c \
    $(SRC_DIR)/port_utils.c \
    $(SRC_DIR)/scanner_utils.c

OBJS       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET     = $(BIN_DIR)/escaner

# Fuente del tester y su binario
TEST_SRC   = $(TEST_DIR)/test_scanner.c
TEST_OBJ   = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRC))
TEST_BIN   = $(BIN_DIR)/test_scanner

.PHONY: all clean run test

all: $(TARGET) $(TEST_BIN)

# ---------- Regla para el escáner ----------
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- Regla para el tester ----------
$(TEST_BIN): $(TEST_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $<

$(BUILD_DIR)/test_scanner.o: $(TEST_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- Objetivo para correr el escáner (rango por defecto) ----------
run: all
	@echo "Ejecutando escáner (rango por defecto 1–6000)..."
	@$(TARGET)

# ---------- Objetivo para pruebas automáticas ----------
test: all
	@echo "Iniciando test_scanner en segundo plano..."
	@$(TEST_BIN) &
	@sleep 1
	@echo "Ejecutando escáner sobre cada puerto de prueba..."
	@for p in 21 22 25 80 31337 4444; do \
	    echo "----- Escaneando puerto $$p -----"; \
	    sudo $(TARGET) $$p $$p; \
	done
	@echo "Matando proceso test_scanner..."
	@pkill -f test_scanner || true
