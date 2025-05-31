# Variables
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -pthread
TARGET = ui_guard

# Subdirectorios que deben ser compilados
SUBDIRS := $(wildcard */)
SUBDIRS := $(filter-out build/ bin/ tests/,$(SUBDIRS))  # excluye carpetas auxiliares

.PHONY: all clean run $(SUBDIRS)

# Compila ui_guard y los subproyectos
all: $(SUBDIRS) $(TARGET)

$(TARGET): ui_guard.c
	$(CC) $(CFLAGS) ui_guard.c -o $(TARGET) $(LDFLAGS)

# Compilar en subdirectorios
$(SUBDIRS):
	@echo "📦 Compilando en $@..."
	@$(MAKE) -C $@

# Ejecuta el programa principal
run: $(TARGET)
	@echo "🚀 Ejecutando interfaz principal: ./$(TARGET)"
	@./$(TARGET)

# Limpia todo
clean:
	@echo "🧹 Limpiando todo..."
	@rm -f $(TARGET)
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
