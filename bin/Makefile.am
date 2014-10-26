# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for programs
# that get installed as part of Quickscope.

qs_LDADD = ../libquickscope.la
AM_CFLAGS = -I.. $(gtk_3_CFLAGS)

bin_PROGRAMS = qs_launcher qs_circle qs_sin

qs_launcher_SOURCES = $(srcdir)/../launcher.c
qs_launcher_LDADD = $(gtk_3_LIBS)

qs_circle_SOURCES = circle.c
qs_circle_LDADD = $(qs_LDADD)

qs_sin_SOURCES = sin.c
qs_sin_LDADD = $(qs_LDADD) $(gtk_3_LIBS) -lm

