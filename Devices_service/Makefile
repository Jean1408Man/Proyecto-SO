###############################################################################
#  Makefile – Proyecto “Devices_service”
#
#  Compila todos los .c que haya en src/ y deja el ejecutable en el raíz
#  de Devices_service.  Usa OpenSSL (‑lcrypto) para SHA‑256.
#
#  Objetivos principales:
#     make          → compila en modo optimizado (-O2)
#     make debug    → compila con símbolos de depuración (-g)
#     make clean    → borra objetos y ejecutables
#     make test     → compila y lanza los tests que haya en tests/
###############################################################################

# ──────── Variables configurables ────────────────────────────────────────────
CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
CPPFLAGS ?= -Iinclude             # busca cabeceras locales
LDFLAGS  ?= -lcrypto              # OpenSSL (SHA‑256)
TARGET    = devices_service       # nombre del binario principal

# Si quieres compilar con libudev más adelante:
# LDFLAGS += -ludev

# ──────── Rutas y archivos automáticos ───────────────────────────────────────
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c, build/%.o, $(SRC))

# tests : recoge todo .c en tests/ (por ahora opcional)
TEST_SRC := $(wildcard tests/*.c)
TEST_OBJ := $(patsubst tests/%.c, build/%.test.o, $(TEST_SRC))
TEST_BIN := tests/run_tests

# Crea la carpeta build/ si no existe (silenciosamente)
$(shell mkdir -p build)

# ──────── Reglas genéricas ───────────────────────────────────────────────────
all: $(TARGET)

# Regla para objetos dentro de build/
build/%.o: src/%.c include/*.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Enlazado final
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	@echo "✓ Generado $(TARGET)"

# ──────── Objetivos auxiliares ───────────────────────────────────────────────
# Modo depuración: recompila con -g -O0
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0
debug: clean all

clean:
	@echo "Limpiando..."
	@rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TEST_BIN)

# ──────── Tests (opcional) ───────────────────────────────────────────────────
$(TEST_BIN): $(TEST_OBJ) $(filter-out src/main.c,$(SRC))
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

test: $(TEST_BIN)
	@echo "── Ejecutando tests ──"
	@./$(TEST_BIN)

.PHONY: all debug clean test
