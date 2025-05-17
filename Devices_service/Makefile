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

TEST_SRC := $(wildcard tests/*.c)
TEST_OBJ := $(patsubst tests/%.c, build/%.test.o, $(TEST_SRC))
TEST_BIN := tests/tester

# Crea la carpeta build/ si no existe (silenciosamente)
$(shell mkdir -p build)

# ──────── Reglas genéricas ───────────────────────────────────────────────────
all: $(TARGET)

build/%.o: src/%.c include/*.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.test.o: tests/%.c include/*.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	@echo "✓ Generado $(TARGET)"

# ──────── Objetivos auxiliares ───────────────────────────────────────────────
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0
debug: clean all

clean:
	@echo "Limpiando..."
	@rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TEST_BIN)

$(TEST_BIN): build/tester.test.o $(filter-out src/main.c,$(SRC))
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Ejecuta el tester principal
test: $(TEST_BIN)
	@echo "── Ejecutando tester de dispositivos ──"
	sudo ./$<

.PHONY: all debug clean test