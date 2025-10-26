CC=gcc
CFLAGS=-Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS=-shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup
SOURCES=memory_tests.c nand.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=memory_tests.h nand.h
LIBRARY=libnand.so
EXAMPLE=nand_example

.PHONY: all clean

all: $(LIBRARY) $(EXAMPLE)

$(LIBRARY): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(EXAMPLE): nand_example.o $(LIBRARY)
	$(CC) -o $@ $< -L. -lnand

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(LIBRARY) $(EXAMPLE) nand_example.o main.o
