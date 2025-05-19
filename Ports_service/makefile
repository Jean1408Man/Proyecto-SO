# === CONFIGURACIÓN ===
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0` -lpthread

# === ESTRUCTURA DE CARPETAS ===
SRCDIR = src
OBJDIR = build
BINDIR = bin

# === ARCHIVOS ===
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
TARGET = $(BINDIR)/escaner

# === REGLAS ===

# Regla por defecto
all: $(TARGET)

# Requiere carpetas antes de compilar ejecutable
$(TARGET): | $(OBJDIR) $(BINDIR)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✅ Compilación completada. Ejecuta: ./$(TARGET)"

# Compilar archivos .c a .o en build/
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Crear carpetas si no existen
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Ejecutar binario (build + run)
run: all
	@echo "🚀 Ejecutando escaner:"
	./$(TARGET)

# Limpiar archivos generados
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "🧹 Limpieza completada."

.PHONY: all clean run
