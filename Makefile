CC = gcc #-g #-O2 #-Wall #-Wextra
CFLAGS = -I./deps -I./src
LDFLAGS = -lglfw -lGL -lpthread -lGLEW -lm -ldl
TARGET = ./grafenic
SOURCES = ./src/main.c
# Default target
all: $(TARGET)
# Build rules
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
# Clean rule
clean:
	rm -f $(TARGET)
# Run rule
run: clean
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
	$(TARGET)
# Run with debugger rule
debug: clean
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
	gdb $(TARGET)
# Overlay rule
.PHONY: clean
