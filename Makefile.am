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

EXTRA_DIST =\
 imgSaveImage.png\
 imgSaveImage.xpm\
 quickscope.ico\
 quickscope.xpm\
 quickscope.png\
 bootstrap\
 Repo_clean

SUBDIRS = . bin

if QS_TESTS
SUBDIRS += tests
endif


BUILT_SOURCES =\
 debug.h\
 quickscope.h

# The .xpm files are built but come in a tarball
# .xpm files are not checked into the repository

if USE_CONVERT
BUILT_SOURCES +=\
 imgSaveImage.xpm\
 quickscope_32.xpm\
 quickscope.ico
endif


lib_LTLIBRARIES = libquickscope.la

libquickscope_la_SOURCES =\
 adjuster.c\
 adjuster.h\
 adjusterBoolean.c\
 adjusterFloat.c\
 adjusterGroup.c\
 adjusterSelector.c\
 adjuster_priv.h\
 app.c\
 app.h\
 assert.c\
 assert.h\
 base.h\
 config.h\
 controller.c\
 controller.h\
 controller_priv.h\
 debug.h\
 drawsync.c\
 drawsync_priv.h\
 fd.c\
 fd.h\
 imgSaveImage.xpm\
 interval.c\
 interval.h\
 iterator.c\
 iterator.h\
 quickscope.xpm\
 source.c\
 source.h\
 source_priv.h\
 group.c\
 group.h\
 saw.c\
 sin.c\
 swipe.c\
 swipe_priv.h\
 sweep.c\
 sweep.h\
 timer.c\
 timer.h\
 trace.c\
 trace.h\
 trace_priv.h\
 win.c\
 win.h\
 win_cb_configure.c\
 win_drawBackground.c\
 win_fadeDraw.c\
 win_keyPress.c\
 win_makeGtkWidgets.c\
 win_priv.h\
 win_savePNG.c\
 win_setGrid.c

noinst_LTLIBRARIES = libadjD.la libadjLD.la libadjInt.la
libadjD_la_SHORTNAME = adjD
libadjLD_la_SHORTNAME = adjLD
libadjInt_la_SHORTNAME = adjInt
libadjD_la_SOURCES = adjusterFloat.c adjuster.h adjuster_priv.h
libadjLD_la_SOURCES = adjusterFloat.c adjuster.h adjuster_priv.h
libadjInt_la_SOURCES = adjusterFloat.c adjuster.h adjuster_priv.h
libadjD_la_CFLAGS = -DDOUBLE $(gtk_3_CFLAGS)
libadjLD_la_CFLAGS = -DLONG_DOUBLE $(gtk_3_CFLAGS)
libadjInt_la_CFLAGS = -DINT $(gtk_3_CFLAGS)

libquickscope_la_SHORTNAME = qs
libquickscope_la_LIBADD = libadjD.la libadjLD.la libadjInt.la $(gtk_3_LIBS) $(sndfile_LIBS) -lX11 -ldl
libquickscope_la_CFLAGS = $(gtk_3_CFLAGS)
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
quickscope_headers =\
 top_quickscope.h\
 debug.h\
 base.h\
 adjuster.h\
 app.h\
 assert.h\
 drawsync.h\
 controller.h\
 fd.h\
 group.h\
 interval.h\
 source.h\
 iterator.h\
 sweep.h\
 timer.h\
 trace.h\
 win.h


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

quickscope.h: Makefile $(quickscope_headers)
	echo "/* This is a generated file */" > $@
	cat $(quickscope_headers) >> $@

hash: sha1 sha256 sha512
sha1:   $(gzfile).sha1   $(bz2file).sha1   $(xzfile).sha1
sha256: $(gzfile).sha256 $(bz2file).sha256 $(xzfile).sha256
sha512: $(gzfile).sha512 $(bz2file).sha512 $(xzfile).sha512

dist-hook:
	rm -f *.sha1 *.sha256 *.sha512
 
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

