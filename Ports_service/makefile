# Makefile para Processes_Service

# Nombres de targets
TARGET_LIB   := libprocmon.a
TARGET_DEMO  := pm_service

# Directorios
SRC_DIR      := src
INC_DIR      := include
OBJ_DIR      := build
TEST_DIR     := tests

# Herramientas
CC           := gcc
CFLAGS       := -std=c11 -Wall -Wextra -I$(INC_DIR)
AR           := ar
ARFLAGS      := rcs

# Fuentes y objetos
SOURCES      := $(wildcard $(SRC_DIR)/*.c)
OBJECTS      := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS         := $(OBJECTS:.o=.d)

# Test sources
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.test.o,$(TEST_SOURCES))
TEST_BINS    := $(patsubst $(TEST_DIR)/%.c,$(TEST_DIR)/%,$(TEST_SOURCES))

.PHONY: all demo run clean tests

all: $(TARGET_LIB) $(TARGET_DEMO)

# Librería estática
$(TARGET_LIB): $(OBJECTS)
    $(AR) $(ARFLAGS) $@ $^

# Ejecutable principal
$(TARGET_DEMO): $(TARGET_LIB)
    $(CC) $(CFLAGS) -o $@ $(OBJECTS) -L. -lprocmon

# Compilar objetos de src/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
    @mkdir -p $(OBJ_DIR)
    $(CC) $(CFLAGS) -MMD -c $< -o $@

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
	@sleep 15
	@echo "Finalizó el tiempo de espera para escaneo."
	@echo "Matando proceso test_scanner..."
	@pkill -f test_scanner || true

# ---------- Limpiar archivos generados ----------
clean:
    @rm -rf $(OBJ_DIR) $(TARGET_LIB) $(TARGET_DEMO) *.d $(TEST_BINS)