# make         # Compila todo
# ./bin/escaner  # Ejecuta el binario
# make clean   # Limpia los archivos generados


# === CONFIGURACIÓN ===
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0` -lpthread

# === ARCHIVOS ===
SRCDIR = src
OBJDIR = build
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
TARGET = $(BINDIR)/escaner

# === REGLAS ===
all: $(TARGET)

# Crea bin y build si no existen
$(TARGET): | $(BINDIR) $(OBJDIR)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✅ Compilación completada. Ejecuta: ./$(TARGET)"

# Compilación de objetos
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Carpetas
$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Limpieza
clean:
	@echo "🧹 Limpiando..."
	@rm -rf build/*.o $(TARGET)

.PHONY: all clean

# Pruebas automatizadas
test: all
	@echo "🧪 Ejecutando pruebas..."
	@chmod +x test_todo.sh
	@./test_todo.sh
