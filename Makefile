# We DO NOT hardcode the compiler (e.g. CC=gcc)
# Nix will automagically inject the correct cross-compiler into the $CC env variable

CFLAGS += -Wall -O2
LDFLAGS += -lz -lhiredis

TARGET = redis-acq400

# default rule run by standard 'make' or Nix's default buildPhase
all: $(TARGET)

$(TARGET): main.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

# Nix will run 'make install' during its installPhase
# Use PREFIX so Nix can tell Makefile exactly where /nix/store output path is
PREFIX ?= /usr/local

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin

clean:
	rm -f *.o $(TARGET)
