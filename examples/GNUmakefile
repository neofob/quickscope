# GNUmakefile is the default GNU make make file.
# Think of GNUmakefile as makefile, but with GNU make extensions

# This file uses GNU make extensions.

CFLAGS = -g -Werror -Wall
QS_CFLAGS = $(shell pkg-config --cflags quickscope)
QS_LFLAGS = $(shell pkg-config --libs quickscope)

programs =$(sort $(patsubst %.c, %,\
 $(filter-out run_list.c, $(wildcard *.c))))

.SUFFIXES: .c .o .so

build: $(programs)

.c.o:
	$(CC) -fPIC -c $(CFLAGS) $(QS_CFLAGS) $< -o $@

.o.so:
	$(CC) -shared $(CFLAGS) $(QS_CFLAGS) $< -o $@


$(programs): %:%.o
	$(CC) $(CFLAGS) $^ -o $@ $(QS_LFLAGS)

clean:
	rm -f *.o *.so $(programs)

cleaner: clean
