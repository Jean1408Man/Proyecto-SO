###############################################################################
#  Makefile – Proyecto “Ports_service”
#
#  Compila todos los .c que haya en src/ y deja el ejecutable en la raíz
#  Usa glib-2.0 (hash table) y pthread (hilos).
#
#  Objetivos principales:
#     make          → compila en modo optimizado (-O2)
#     make debug    → compila con símbolos de depuración (-g)
#     make clean    → borra objetos y ejecutables
###############################################################################

# ──────── Configuración ──────────────────────────────────────────────────────
CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
CPPFLAGS ?= -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS  ?= `pkg-config --libs glib-2.0` -lpthread
TARGET    = escaner

# ──────── Archivos fuente y objetos ──────────────────────────────────────────
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c, build/%.o, $(SRC))

# Crea la carpeta build/ si no existe (silenciosamente)
$(shell mkdir -p build)

# ──────── Reglas principales ─────────────────────────────────────────────────
all: $(TARGET)

build/%.o: src/%.c include/*.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	@echo "✓ Binario generado: ./$@"

# ──────── Modos adicionales ──────────────────────────────────────────────────
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0
debug: clean all

clean:
	@echo "🧹 Limpiando..."
	@rm -rf build/*.o $(TARGET)

.PHONY: all debug clean