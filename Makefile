# Variables
CC = gcc -g -O2
CFLAGS = -Wall -I/usr/include/freetype2 -I/usr/include/GL
LDFLAGS = -lglfw -lGL -lGLEW -lm -lX11 -lpthread -lXi -lXrandr -ldl -lfreetype
TARGET = ./Build/Graphene
SOURCES = ./Source/*.c
HEADERS = ./Source/*/*.h

# Default target
all: $(TARGET)

# Build rules
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET)
