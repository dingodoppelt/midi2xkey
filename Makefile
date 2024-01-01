# Makefile für midi2xkey.cpp

CC = g++                   # Der Compiler
CFLAGS = -Wall -std=c++17  # Compiler-Flags
LDFLAGS = $(shell pkg-config --cflags --libs libpipewire-0.3) -ljack -lxdo -lstdc++

# Name der ausführbaren Datei
TARGET = midi2xkey

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
