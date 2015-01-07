# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for programs
# that get installed as part of Quickscope.

qs_LDADD = $(top_builddir)/lib/libquickscope.la -lm
AM_CPPFLAGS = $(GTK_3_CFLAGS)

bin_PROGRAMS =\
 qs_demo_launcher\
 qs_rossler3Wins\
 qs_circle\
 qs_sin\
 qs_alsaCapture\
 qs_urandom\
 qs_ode

qs_demo_launcher_SOURCES = $(top_srcdir)/lib/launcher.c
qs_demo_launcher_LDADD = $(GTK_3_LIBS) -ldl

qs_circle_SOURCES = circle.c
qs_circle_LDADD = $(qs_LDADD)

qs_alsaCapture_SOURCES = alsaCapture.c
qs_alsaCapture_LDADD = $(qs_LDADD)

qs_sin_SOURCES = sin.c
qs_sin_LDADD = $(qs_LDADD)

qs_rossler3Wins_SOURCES = rossler3Wins.c
qs_rossler3Wins_LDADD = $(qs_LDADD)

qs_urandom_SOURCES = urandom.c
qs_urandom_LDADD = $(qs_LDADD)

qs_ode_SOURCES = ode.c
qs_ode_LDADD = $(qs_LDADD)

