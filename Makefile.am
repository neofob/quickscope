# Quickscope - a software oscilloscope
#
# Copyright (C) 2012-2014  Lance Arsenault

# This file is part of Quickscope.
#
# Quickscope is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# Quickscope is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Quickscope.  If not, see <http://www.gnu.org/licenses/>.
#
####################################################################

SUBDIRS = . bin

if QS_TESTS
SUBDIRS += tests
endif

quickscope_lastheaders =\
 Assert.h\
 base.h\
 adjuster.h\
 app.h\
 controller.h\
 group.h\
 source.h\
 iterator.h\
 soundFile.h\
 trace.h\
 win.h\
 rungeKutta.h\
 sourceParticular.h


# The .xpm files are built but come in a tarball.
# .xpm files are not checked into the repository.
# Same for .ico files.

convert_generated_images =\
 imgSaveImage.xpm\
 quickscope_32.xpm\
 quickscope.ico

examples =\
 examples/README\
 examples/GNUmakefile\
 examples/sin.c\
 examples/demo_launcher\
 examples/run_list


EXTRA_DIST =\
 imgSaveImage.png\
 quickscope.png\
 top_quickscope.h\
 bootstrap\
 Repo_clean\
 $(convert_generated_images)\
 $(examples)\
 repo_require.bash


BUILT_SOURCES =\
 debug.h\
 quickscope.h

if USE_CONVERT
BUILT_SOURCES += $(convert_generated_images)
endif



lib_LTLIBRARIES = libquickscope.la

libquickscope_la_SOURCES =\
 alsaCapture.c\
 adjuster.c\
 adjuster.h\
 adjusterBool.c\
 adjusterType.c\
 adjusterGroup.c\
 adjusterSelector.c\
 adjuster_priv.h\
 app.c\
 app.h\
 app_priv.h\
 app_arg.c\
 assert.c\
 Assert.h\
 base.h\
 config.h\
 controller.c\
 controller.h\
 controller_priv.h\
 drawsync.c\
 fd.c\
 imgSaveImage.xpm\
 interval.c\
 idle.c\
 iterator.c\
 iterator.h\
 source.c\
 source_frameRate.c\
 source.h\
 sourceParticular.h\
 source_priv.h\
 group.c\
 group.h\
 saw.c\
 sin.c\
 soundFile.c\
 soundFile.h\
 swipe.c\
 swipe_priv.h\
 sweep.c\
 timer.c\
 timer_priv.h\
 trace.c\
 trace.h\
 trace_priv.h\
 win.c\
 win.h\
 win_cb_configure.c\
 win_drawBackground.c\
 win_fadeDraw.c\
 win_fadeDraw_priv.h\
 win_keyPress.c\
 win_makeGtkWidgets.c\
 win_priv.h\
 win_savePNG.c\
 win_setGrid.c\
 rungeKutta.c\
 rungeKutta.h\
 rk4Source.c\
 lorenz.c\
 rossler.c\
 urandom.c

nodist_libquickscope_la_SOURCES =\
 debug.h

# automake (as of version 1.14.1) has no per-object
# spectfic flags, so making a library for each object
# that requires spectfic CPP flags is a workaround
# that gets us per-object spectfic flags on these
# special objects.

# adjusterType.c is a marco template source file,
# so to speak.  It's default Type is float.
noinst_LTLIBRARIES = libadjD.la libadjLD.la libadjInt.la
libadjD_la_SHORTNAME = adjD
libadjLD_la_SHORTNAME = adjLD
libadjInt_la_SHORTNAME = adjInt
libadjD_la_SOURCES = adjusterType.c adjuster.h adjuster_priv.h
libadjLD_la_SOURCES = adjusterType.c adjuster.h adjuster_priv.h
libadjInt_la_SOURCES = adjusterType.c adjuster.h adjuster_priv.h
libadjD_la_CFLAGS = -DDOUBLE $(GTK_3_CFLAGS)
libadjLD_la_CFLAGS = -DLONG_DOUBLE $(GTK_3_CFLAGS)
libadjInt_la_CFLAGS = -DINT $(GTK_3_CFLAGS)

libquickscope_la_SHORTNAME = qs
libquickscope_la_LIBADD =\
 libadjD.la\
 libadjLD.la\
 libadjInt.la\
 $(GTK_3_LIBS)\
 $(SNDFILE_LIBS)\
 $(ALSA_LIBS)\
 -lX11\
 -ldl\
 -lm
libquickscope_la_CFLAGS = $(GTK_3_CFLAGS) $(SNDFILE_CFLAGS) $(ALSA_CFLAGS)
libquickscope_la_LDFLAGS = -export-symbols-regex '^qs'


# For making libquickscope.so executable if we are using GCC (g++) and
# glibc which we tested by compiling with the gnu/lib-names.h file.
if HAVE_GNU_LIB_NAMES_H
libquickscope_la_SOURCES  += lib_main.c
libquickscope_la_LDFLAGS  += -shared -Wl,-e,_lib_main
libquickscope_la_CFLAGS   += -fPIC -shared
else
EXTRA_DIST += lib_main.c
endif


pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = quickscope.pc

include_HEADERS = quickscope.h


gzfile  = $(distdir).tar.gz
bz2file = $(distdir).tar.bz2
xzfile  = $(distdir).tar.xz


SUFFIXES = .html .bz2 .xz .gz \
.gz.sha1 .gz.sha256 .gz.sha512 \
.bz2.sha1 .bz2.sha256 .bz2.sha512 \
.xz.sha1 .xz.sha256 .xz.sha512

if USE_CONVERT
SUFFIXES += .png .xpm .ico _32.xpm
endif



.PHONY: hash sha1 sha256 sha512

if USE_CONVERT
.png_32.xpm:
	convert $< -resize 32x32 $@
	mv $@ $@.in
	sed -e 's/static.*\schar/static const char/g' $@.in > $@
	rm $@.in
.png.ico:
	convert $< -resize 32x32 $@
.png.xpm:
	convert $< $@
	mv $@ $@.in
	sed -e 's/static.*\schar/static const char/g' $@.in > $@
	rm $@.in
endif



debug.h:
	echo "/* This is a generated file */" > $@
	echo "#define _QS_DEBUG_H_" >> $@
if QS_DEBUG
	echo "#define QS_DEBUG 1" >> $@
else
	echo "/*  QS_DEBUG is not defined in this build */" >> $@
endif


quickscope.h: Makefile $(srcdir)/top_quickscope.h debug.h $(quickscope_lastheaders)
	echo "/* This is a generated file */" > $@
	cat $(srcdir)/top_quickscope.h >> $@
	cat debug.h >> $@
	for i in $(quickscope_lastheaders) ; do \
           echo "/*********** Start from file: $$i **************/" >> $@; \
           cat $(srcdir)/$$i >> $@; \
           echo "/*********** End from file: $$i ****************" >> $@; \
           echo " ***************************************************/" >> $@; \
        done


hash: sha1 sha256 sha512
sha1:   $(gzfile).sha1   $(bz2file).sha1   $(xzfile).sha1
sha256: $(gzfile).sha256 $(bz2file).sha256 $(xzfile).sha256
sha512: $(gzfile).sha512 $(bz2file).sha512 $(xzfile).sha512

 
.gz.gz.sha1:
	sha1sum $< > $@
.gz.gz.sha256:
	sha256sum $< > $@
.gz.gz.sha512:
	sha512sum $< > $@

.bz2.bz2.sha1:
	sha1sum $< > $@
.bz2.bz2.sha256:
	sha256sum $< > $@
.bz2.bz2.sha512:
	sha512sum $< > $@

.xz.xz.sha1:
	sha1sum $< > $@
.xz.xz.sha256:
	sha256sum $< > $@
.xz.xz.sha512:
	sha512sum $< > $@

$(gzfile):
	$(MAKE) dist-gzip

$(bz2file):
	$(MAKE) dist-bzip2

$(xzfile):
	$(MAKE) dist-xz
