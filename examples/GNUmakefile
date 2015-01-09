# GNUmakefile is the default GNU make make file.
# Think of GNUmakefile as makefile, but with GNU make extensions

# This file uses GNU make extensions.

# I like bash not sh
SHELL = /bin/bash

CFLAGS = -g -Werror -Wall
QS_CFLAGS = $(shell pkg-config --cflags quickscope)
QS_LFLAGS = $(shell pkg-config --libs quickscope) -lm

QS_INSTALLED_HFILE =\
 $(shell pkg-config --variable=includedir quickscope)/quickscope.h
QS_INSTALLED_LIB =\
 $(shell pkg-config --variable=libdir quickscope)/libquickscope.so

programs =$(sort $(patsubst %.c, %,\
 $(filter-out run_list.c, $(wildcard *.c))))

program_files = $(wildcard *.c)

.SUFFIXES: .c .o .so

build: $(programs) run_list

run_list: $(programs)
	echo "# this is a generated file" > $@
	echo -e "$(addprefix \n./,$(programs))" >> $@

%.o: %.c $(QS_INSTALLED_HFILE)
	$(CC) $(CFLAGS) $(QS_CFLAGS) -c $< -o $@


$(programs): %:%.o $(QS_INSTALLED_LIB)
	$(CC) $(CFLAGS) $(filter-out $(QS_INSTALLED_LIB),$^) -o $@ $(QS_LFLAGS)

clean:
	rm -f *.o $(programs) run_list

cleaner: clean
