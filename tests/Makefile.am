# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for
# development test programs.
# They are not so useful for regular
# quickscope users.

noinst_PROGRAMS = hello source circle saw non_master_print


hello_SOURCES = hello.c

qs_LDADD = ../libquickscope.la
AM_CFLAGS = -I.. $(gtk_3_CFLAGS)

circle_SOURCES = circle.c
circle_LDADD = $(qs_LDADD)

source_SOURCES = source.c
source_LDADD = $(qs_LDADD)

saw_SOURCES = saw.c
saw_LDADD = $(qs_LDADD)

non_master_print_SOURCES = non_master_print.c
non_master_print_LDADD = $(qs_LDADD)

