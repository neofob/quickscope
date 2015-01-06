# Quickscope - a software oscilloscope
# Copyright (C) 2012-2014  Lance Arsenault
# GNU General Public License version 3
#############################################

# This directory has source code for
# development test programs.
# They are not necassarly so useful for
# regular quickscope users. 

BUILT_SOURCES =\
 rossler3Wins.c\
 alsaCapture.c

noinst_PROGRAMS =\
 tests_launcher\
 hello\
 qsSource\
 soundFile_print\
 circle\
 saw_print\
 non_master_print\
 sin\
 soundFile\
 rk4_print\
 ode_print\
 ode\
 rossler3Wins\
 urandom\
 alsa_info\
 alsa_capture_print\
 idle\
 alsaCapture\
 qsGtkWidget\
 controller\
 app_arg



qs_LDADD = ../libquickscope.la 

AM_CFLAGS = $(GTK_3_CFLAGS)

# tests_launcher does not need libquickscope
# but it does need gtk-3
tests_launcher_SOURCES = $(srcdir)/../launcher.c
tests_launcher_LDADD = $(GTK_3_LIBS) -ldl
tests_launcher_CFLAGS = -DTESTS $(GTK_3_CFLAGS)

qsGtkWidget_SOURCES = qsGtkWidget.c
qsGtkWidget_LDADD = $(qs_LDADD) $(GTK_3_LIBS) -lX11
qsGtkWidget_CFLAGS = -DTESTS $(GTK_3_CFLAGS)

hello_SOURCES = hello.c

circle_SOURCES = circle.c quickscope.h
circle_LDADD = $(qs_LDADD)

idle_SOURCES = idle.c quickscope.h
idle_LDADD = $(qs_LDADD)

qsSource_SOURCES = qsSource.c quickscope.h
qsSource_LDADD = $(qs_LDADD)

saw_print_SOURCES = saw_print.c quickscope.h
saw_print_LDADD = $(qs_LDADD)

non_master_print_SOURCES = non_master_print.c quickscope.h
non_master_print_LDADD = $(qs_LDADD)

sin_SOURCES = sin.c
sin_LDADD = $(qs_LDADD)

soundFile_SOURCES = soundFile.c quickscope.h
soundFile_LDADD = $(qs_LDADD)

soundFile_print_SOURCES = soundFile_print.c quickscope.h
soundFile_print_LDADD = $(qs_LDADD)

rk4_print_SOURCES = rk4_print.c quickscope.h
rk4_print_LDADD = $(qs_LDADD)

ode_print_SOURCES = ode_print.c quickscope.h
ode_print_LDADD = $(qs_LDADD)

ode_SOURCES = ode.c quickscope.h
ode_LDADD = $(qs_LDADD)

rossler3Wins_SOURCES = rossler3Wins.c quickscope.h
rossler3Wins_LDADD = $(qs_LDADD)

urandom_SOURCES = urandom.c quickscope.h
urandom_LDADD = $(qs_LDADD)

alsa_info_SOURCES = alsa_info.c quickscope.h
alsa_info_LDADD = $(qs_LDADD) $(ALSA_LIBS)
alsa_info_CFLAGS = $(GTK_3_CFLAGS) $(ALSA_CFLAGS)

alsa_capture_print_SOURCES = alsa_capture_print.c
alsa_capture_print_LDADD = $(qs_LDADD) $(ALSA_LIBS) $(GTK_3_LIBS)
alsa_capture_print_CFLAGS = $(GTK_3_CFLAGS) $(ALSA_CFLAGS)

alsaCapture_SOURCES = alsaCapture.c quickscope.h
alsaCapture_LDADD = $(qs_LDADD)

controller_SOURCES = controller.c quickscope.h
controller_LDADD = $(qs_LDADD)

app_arg_SOURCES = app_arg.c quickscope.h
app_arg_LDADD = $(qs_LDADD)




rossler3Wins.c: $(top_srcdir)/bin/rossler3Wins.c
	$(LN_S) $< $@

alsaCapture.c: $(top_srcdir)/bin/alsaCapture.c
	$(LN_S) $< $@
