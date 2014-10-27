# GNUmakefile is the default GNU make make file.
# Think of GNUmakefile as makefile, but with GNU make extensions

# This file uses GNU make extensions.

CFLAGS = -g -Werror -Wall
QS_CFLAGS = $(shell pkg-config --cflags quickscope)
QS_LFLAGS = $(shell pkg-config --libs quickscope)

programs = $(patsubst %.c, %, $(wildcard *.c))

.c.o:
	$(CC) -c $(CFLAGS) $(QS_CFLAGS) $< -o $@

build: $(programs)


$(programs): %:%.o
	$(CC) $(CFLAGS) $^ -o $@ $(QS_LFLAGS)

clean:
	rm -f *.o $(programs)

cleaner: clean
