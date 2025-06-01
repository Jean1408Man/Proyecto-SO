# === CONFIGURACIÓN ===
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0` -lpthread

# === DIRECTORIOS ===
SRCDIR = src
OBJDIR = build
BINDIR = bin
TESTDIR = tests

# === ARCHIVOS ===
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
TARGET = $(BINDIR)/escaner

TEST_SCANNER_SRC = $(TESTDIR)/test_scanner.c
TEST_SCANNER_BIN = $(BINDIR)/test_scanner

# === REGLAS ===
all: $(TEST_SCANNER_BIN) $(TARGET)

# Ejecuta el sistema completo: primero test_scanner, luego escaner
run: all
	@echo "🚀 Ejecutando prueba con puertos simulados..."
	@$(TEST_SCANNER_BIN) & \
	sleep 1 && ./$(TARGET); \
	wait

# Binario principal
$(TARGET): | $(BINDIR) $(OBJDIR)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✅ Compilación de escaner completada. Ejecuta: ./$(TARGET)"

# Binario test_scanner
$(TEST_SCANNER_BIN): $(TEST_SCANNER_SRC) | $(BINDIR)
	$(CC) $(TEST_SCANNER_SRC) -o $@ $(LDFLAGS)
	@echo "🛠️  test_scanner compilado como $(TEST_SCANNER_BIN)"

# Compilación de objetos
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Carpetas necesarias
$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Limpieza
clean:
	@echo "🧹 Limpiando..."
	@rm -rf $(OBJDIR)/*.o $(BINDIR)/*

.PHONY: all clean run
