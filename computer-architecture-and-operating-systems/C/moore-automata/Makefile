# Wykonanie polecenia make libma.so spowoduje skompilowanie biblioteki.
# Wykonanie polecenia make example spowoduje skompilowanie 
# programu przykładowego i biblioteki.
# Użycie programu przykładowego: ./ma_example <nazwa testu>
# W wypadku błędu mówiącego o braku libma.so, należy użyć 
# LD_LIBRARY_PATH=. ./ma_example <nazwa testu>.
# Wykonanie polecenia make clean spowoduje usunięcie 
# plików wygenerowanych przez kompilację.
CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
          -Wl,--wrap=reallocarray -Wl,--wrap=free \
		  -Wl,--wrap=strdup -Wl,--wrap=strndup

LIB_SRCS = ma.c memory_tests.c
LIB_HEADERS = ma.h memory_tests.h
LIB_TARGET = libma.so

# Opcjonalny program przykładowy
EXAMPLE_SRC = ma_example.c
EXAMPLE_TARGET = ma_example

.PHONY: all clean libma.so example

all: libma.so

libma.so: $(LIB_SRCS) $(LIB_HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB_TARGET) $(LIB_SRCS)

example: $(EXAMPLE_SRC) libma.so
	$(CC) $(CFLAGS) -o $(EXAMPLE_TARGET) $(EXAMPLE_SRC) -L. -lma

clean:
	rm -f $(LIB_TARGET) $(EXAMPLE_TARGET) *.o

