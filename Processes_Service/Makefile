# Makefile para Processes_Service

# Nombres de targets
TARGET_LIB   := libprocmon.a
TARGET_DEMO  := pm_demo

# Directorios
SRC_DIR      := src
INC_DIR      := include
OBJ_DIR      := build
TEST_DIR     := tests

# Variables de configuración
WHITELIST_FILE := etc/whitelist.txt
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

.PHONY: all demo run ru tests clean

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

# Compilar objetos de tests/
$(OBJ_DIR)/%.test.o: $(TEST_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Ejecutables de tests
$(TEST_DIR)/%: $(OBJ_DIR)/%.test.o
	$(CC) $(CFLAGS) $< -o $@

# Incluir dependencias generadas
-include $(DEPS)

# Alias para demo
demo: $(TARGET_DEMO)

# Ejecutar el demo con la variable de entorno
run: $(TARGET_DEMO)
	WHITELIST_FILE="$(WHITELIST_FILE)" ./$(TARGET_DEMO) 10.0 50.0 10

# Alias corto para run
ru: run

# Compilar y ejecutar todos los tests
tests: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "==> Ejecutando $$t"; \
		./$$t; \
	done

# Limpieza
clean:
	@rm -rf $(OBJ_DIR) $(TARGET_LIB) $(TARGET_DEMO) *.d $(TEST_BINS)
