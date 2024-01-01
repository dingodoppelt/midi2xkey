# Makefile für midi2keypress.cpp

CC = g++                   # Der Compiler
CFLAGS = -Wall -std=c++11  # Compiler-Flags
LDFLAGS = $(shell pkg-config --cflags --libs libpipewire-0.3) -ljack -lxdo -lstdc++

# Name der ausführbaren Datei
TARGET = midi2keypress

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
