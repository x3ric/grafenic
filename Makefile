# Variables
CC = gcc #-g -O2 -Wextra
CFLAGS = -Wall -I/usr/include/freetype2 -I/usr/include/GL
LDFLAGS = -lglfw -lGL -lGLEW -lm -lX11 -lpthread -lXi -lXrandr -ldl -lfreetype
TARGET = ./graphene
SOURCES = ./src/*.c
HEADERS = ./src/*/*.h

# Default target
all: $(TARGET)

# Build rules
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET)
