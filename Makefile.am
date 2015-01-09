# Quickscope - a software oscilloscope
#
# Copyright (C) 2012-2015  Lance Arsenault

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

SUBDIRS = . lib bin

if QS_TESTS
SUBDIRS += tests
endif

# adding examples
# for i in examples/[a-zA-Z]* examples/[0-9][0-9]_*.c ; do echo -e " $i\\" ; done
examples =\
 examples/demo_launcher\
 examples/GNUmakefile\
 examples/README\
 examples/00_sin.c\
 examples/05_sin_lessComments.c\
 examples/08_saw.c\
 examples/13_lissajous.c\
 examples/24_lorenz.c\
 examples/40_my_ode_source.c\
 examples/44_my_ode_source_2win.c\
 examples/55_pulseCapture_highLatency.c\
 examples/60_alsaCapture.c\
 examples/65_rossler_3win.c


EXTRA_DIST =\
 bootstrap\
 Repo_clean\
 $(examples)\
 repo_require.bash\
 README.html\
 README.md

 BUILT_SOURCES =

if USE_MARKED
 BUILT_SOURCES += README.html
endif



bz2file = $(distdir).tar.bz2


SUFFIXES = .html .bz2 .bz2.sha512

if USE_MARKED
 SUFFIXES += .md
endif



.PHONY: hash sha512

if USE_MARKED
.md.html:
	marked $< > $@
endif


hash: sha512
sha512: $(bz2file).sha512


.bz2.bz2.sha512:
	sha512sum $< > $@

$(bz2file):
	$(MAKE) dist-bzip2

