# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for
# development test programs.
# They are not so useful for regular
# quickscope users. 

EXTRA_DIST =\
 Quickplot.bash

noinst_PROGRAMS =\
 tests_launcher\
 hello\
 source\
 soundFile_print\
 circle\
 saw_print\
 non_master_print\
 sin\
 soundFile\
 rk4_print\
 ode_print\
 ode\
 ode_3wins

noinst_SCRIPTS =\
 Quickplot

BUILT_SOURCES =\
 Quickplot


qs_LDADD = ../libquickscope.la 

AM_CFLAGS = -I.. $(gtk_3_CFLAGS)

tests_launcher_SOURCES = $(srcdir)/../launcher.c
tests_launcher_LDADD = $(gtk_3_LIBS) -ldl
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

sin_SOURCES = sin.c
sin_LDADD = $(qs_LDADD)

soundFile_SOURCES = soundFile.c
soundFile_LDADD = $(qs_LDADD)

soundFile_print_SOURCES = soundFile_print.c
soundFile_print_LDADD = $(qs_LDADD)

rk4_print_SOURCES = rk4_print.c
rk4_print_LDADD = $(qs_LDADD)

ode_print_SOURCES = ode_print.c
ode_print_LDADD = $(qs_LDADD)

ode_SOURCES = ode.c
ode_LDADD = $(qs_LDADD)

ode_3wins_SOURCES = ode_3wins.c
ode_3wins_LDADD = $(qs_LDADD)





Quickplot: Quickplot.bash Makefile
	echo "#!/bin/bash" > $@
	echo "# This is a generated file" >> $@
	cat $< >> $@
	chmod 755 $@
