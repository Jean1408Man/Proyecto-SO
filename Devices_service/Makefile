# ------------------------------------------------------------------------
# Makefile para Devices_service (MatCom Guard – detección de dispositivos)
# ------------------------------------------------------------------------

# Compilador y opciones
CC       := gcc
CFLAGS   := -Wall -Wextra -Iinclude -g

# Directorios
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

# Ejecutable final
TARGET   := $(BUILDDIR)/devices_service

# Busca todos los .c en src/
SOURCES  := $(wildcard $(SRCDIR)/*.c)
# Convierte src/foo.c → build/foo.o
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

# Target por defecto: compilar todo
all: $(TARGET)

# Enlaza el ejecutable a partir de los objetos
$(TARGET): $(OBJECTS) | $(BUILDDIR)
	@echo "🔗 Enlazando ejecutable: $@"
	$(CC) $(CFLAGS) $^ -o $@

# Regla genérica: compilar cada .c en su .o correspondiente
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "⚙️  Compilando $< → $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Asegura que exista el directorio build
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Limpia todo lo generado
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -rf $(BUILDDIR)

.PHONY: all clean
