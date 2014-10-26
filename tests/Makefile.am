# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for
# development test programs.
# They are not so useful for regular
# quickscope users.

noinst_PROGRAMS =\
 tests_launcher\
 hello\
 source\
 circle\
 saw_print\
 non_master_print\
 sin_sweep

noinst_SCRIPTS =\
 quickplot.bash



qs_LDADD = ../libquickscope.la
AM_CFLAGS = -I.. $(gtk_3_CFLAGS)

tests_launcher_SOURCES = $(srcdir)/../launcher.c
tests_launcher_LDADD = $(gtk_3_LIBS)
tests_launcher_CFLAGS = -DTESTS $(gtk_3_CFLAGS)

hello_SOURCES = hello.c

circle_SOURCES = circle.c
circle_LDADD = $(qs_LDADD)

source_SOURCES = source.c
source_LDADD = $(qs_LDADD)

saw_print_SOURCES = saw_print.c
saw_print_LDADD = $(qs_LDADD)

non_master_print_SOURCES = non_master_print.c
non_master_print_LDADD = $(qs_LDADD)

sin_sweep_SOURCES = sin_sweep.c
sin_sweep_LDADD = $(qs_LDADD)
