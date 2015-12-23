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


# require autoconf version
AC_PREREQ(2.69)

#################################################################
############### Configure a release here ########################
#################################################################


# AC_INIT( PACKAGE, VERSION, BUGREPORT, TARNAME , URL)
AC_INIT([quickscope], 0.1.0,
        [https://github.com/lanceman2/quickscope],
        [quickscope],
        [https://github.com/lanceman2/quickscope])


# LIB_VERSION=LIB_CURRENT:LIB_REVISION:LIB_AGE

LIB_CURRENT=0
LIB_REVISION=0
LIB_AGE=0

#
# See LIB_VERSION notes at the very bottom of this file
#


#################################################################
############## END: Configure a release here ####################
#################################################################


# Stuff that may be configured to change constants that
# get compiled in.

DEFAULT_GRID_FONT="Sans 10"



################################################################
# setting many variables before  AC_PROG_*
################################################################

AC_CONFIG_HEADERS(lib/config.h)

#AM_INIT_AUTOMAKE(option1 option2 ...)
#AM_INIT_AUTOMAKE([subdir-objects] dist-bzip2 dist-xz)
#AM_INIT_AUTOMAKE(dist-bzip2 dist-xz)
#AC_CONFIG_MACRO_DIR([m4])

#AM_INIT_AUTOMAKE(-Wall -Werror dist-bzip2 no-dist-gzip subdir-objects)
AM_INIT_AUTOMAKE(-Wall dist-bzip2 no-dist-gzip)
AM_PROG_AR
LT_INIT
AC_PROG_CC


#################################################################

LIB_VERSION="${LIB_CURRENT}:${LIB_REVISION}:${LIB_AGE}"

################################################################

# initialize the error flag
error=no

######################## REPO BUILD #########################

enable_repobuild_default=no
# using `dirname $0` lets this work if the build dir is not src dir
if test -f "`dirname $0`"/.git/config ; then
  enable_repobuild_default=yes
fi

AC_ARG_ENABLE([repobuild],
    AS_HELP_STRING([--enable-repobuild],
        [if you wish to build Quickscope from\
 just the repository source files.\
 The default is auto.]),
 [enable_repobuild="$enableval"],
 [enable_repobuild="$enable_repobuild_default"])


##################### imagemagick convert ###################


# convert is used to make .xpm files from .png files.

USE_CONVERT=no

if test "$enable_repobuild" != "no"; then
  USE_CONVERT=yes
  AC_CHECK_PROG(HAVE_CONVERT, convert, [yes], [no])

  if test x$HAVE_CONVERT != xyes; then
    error=yes
    AC_MSG_WARN([
        For building $PACKAGE_NAME for the git repository
        convert from ImageMagick is required to be in your PATH
        homepage: http://www.imagemagick.org/])
  fi
fi

AM_CONDITIONAL([USE_CONVERT], [test x$USE_CONVERT = xyes])


##################### marked mark down program ###################

USE_MARKED=no

if test "$enable_repobuild" != "no"; then
  USE_MARKED=yes
  AC_CHECK_PROG(HAVE_MARKED, convert, [yes], [no])

  if test x$HAVE_MARKED != xyes; then
    error=yes
    AC_MSG_WARN([
        For building $PACKAGE_NAME from the git repository
        marked is required to be in your PATH
        homepage: https://github.com/chjj/marked.git])
  fi
fi

AM_CONDITIONAL([USE_MARKED], [test x$USE_MARKED = xyes])


##############################################################


PKG_CHECK_MODULES([GTK_3], [gtk+-3.0 >= 3.0.12],
    [],
    [error=yes
     AC_MSG_WARN([
        gtk+-3.0 version >= 3.0.12 is required
        homepage: http://www.gtk.org/])
    ]
)

PKG_CHECK_MODULES([SNDFILE], [sndfile >= 1.0.5],
    [],
    [error=yes
     AC_MSG_WARN([
        lib sndfile version >= 1.0.5 is required
        homepage: http://www.mega-nerd.com/libsndfile/])
    ]
)

PKG_CHECK_MODULES([ALSA], [alsa >= 1.0.28],
    [],
    [error=yes
     AC_MSG_WARN([
        lib alsa version >= 1.0.28 is required])
    ]
)

PKG_CHECK_MODULES([PULSEAUDIO], [libpulse-simple >= 4.0],
      [],
    [error=yes
     AC_MSG_WARN([
        libpulse-simple >= 5.0 is required
        The PulseAudio The simple PulseAudio API
        http://freedesktop.org/software/pulseaudio
        ])
    ]
)


################################################################
#                 --enable-debug
################################################################

AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug],
    [compile more debugging code into quickscope and libquickscope\
 (default is no debugging code).  You do not want this unless\
 you are developing Quickscope.  Adding this option may make\
 use about 10% more CPU usage.  This is unrelated to the compiler\
 options used.]),
 [enable_debug="$enableval"],
 [enable_debug=no])

case "$enable_debug" in
    y* | Y* )
    debug=yes
    ;;
    * )
    debug=no
    ;;
esac

################################################################
#                 --enable-tests
################################################################

AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--enable-tests],
    [compile tests code (default is no testing code).]),
 [enable_tests="$enableval"],
 [enable_tests=no])

case "$enable_tests" in
    y* | Y* )
    tests=yes
    ;;
    * )
    tests=no
    ;;
esac

################################################################
#    test for gnu/lib-names.h
################################################################

HAVE_GNU_LIB_NAMES_H=no

# This is so we can see if the file gnu/lib-names.h is present and
# works so that we can make the libraries executable. See that this
# compiles will be a good enough test.  Used in
# libquickscope/lib_main.c
AC_MSG_CHECKING([if gnu/lib-names.h is present and works\
 so we can make libraries executable])

# Below the real code requires a "const char my_interp[] __att.... "
# etc, but I don't know how to make a '[' in an autoconf string.

AC_RUN_IFELSE([AC_LANG_SOURCE([[

#include <gnu/lib-names.h>

#ifdef __i386__
# define LIBDIR "/lib/"
#endif

#ifdef __x86_64__
# define LIBDIR "/lib64/"
#endif

#ifndef LIBDIR
# error "The compiler did not define __i386__ or __x86_64__"
#endif

const char my_interp[] __attribute__
    ((section(".interp")))= LIBDIR LD_SO;

int main(void)
{
  /* do something so the compiler does not give a warning */
  char *ptr = (char *) my_interp;
  ptr += 1;
  return 0;
}
]])],
  [AC_MSG_RESULT([yes])]
  HAVE_GNU_LIB_NAMES_H=yes,
  [AC_MSG_RESULT([no])],
  [AC_MSG_RESULT([CROSS-COMPILING])]
)

AM_CONDITIONAL([HAVE_GNU_LIB_NAMES_H],
               [test x$HAVE_GNU_LIB_NAMES_H = xyes])


#################################################################

# AC_SUBST() go into Makefile.am files and other .in files
# AC_DEFINE*() go into config.h

AC_SUBST(DEFAULT_GRID_FONT)
AC_DEFINE_UNQUOTED(DEFAULT_GRID_FONT, "$DEFAULT_GRID_FONT",
        [The default font used when drawing the numbers on the grid])



AC_DEFINE_UNQUOTED(LIB_VERSION, "$LIB_VERSION",
  [Quickscope Library version number])

AC_SUBST(LIB_VERSION)
AC_SUBST(LIB_CURRENT)
AC_SUBST(LIB_REVISION)
AC_SUBST(LIB_AGE)

AM_CONDITIONAL([QS_DEBUG], [test x$debug = xyes])

if test x$debug == xyes ; then
  AC_DEFINE_UNQUOTED(QS_DEBUG,[1],
    [If QS_DEBUG is defined we add extra debug\
 code to quickscope and libquickscope.])
fi


AM_CONDITIONAL([QS_TESTS], [test x$tests = xyes])

if test x$tests == xyes ; then
  AC_DEFINE_UNQUOTED(QS_TESTS,[1],
    [If QS_TESTS we build tests code.])
fi



#################################################################
# We show all warnings to the user before erroring out.  This
# beats having the user fix one error in order to get to the next.
# Instead we show all the warnings (errors) that we can.

if test x$error = xyes ; then
  AC_MSG_ERROR([

  Failed to configure quickscope version $VERSION.
  See above WARNING(s).

])
fi

#################################################################

AC_CONFIG_FILES(
Makefile
lib/Makefile
lib/quickscope.pc
bin/Makefile
tests/Makefile
)

AC_OUTPUT

# you can add spew here

AC_MSG_RESULT([
  ------------------------------------------------------------
    done configuring: $PACKAGE_NAME  version $VERSION
  ------------------------------------------------------------

              extra debug code (--enable-debug): $debug
              build tests code (--enable-tests): $tests
  extra repository targets (--enable-repobuild): $enable_repobuild

                 C Compiler (CC): $CC
       C Compiler Flags (CFLAGS): $CFLAGS
        C Linker Flags (LDFLAGS): $LDFLAGS
         make executable library: $HAVE_GNU_LIB_NAMES_H

  Installation prefix (--prefix): $prefix
                          srcdir: $srcdir
])


######################################################################
#                     LIB_VERSION notes
######################################################################

#  Updating library version interface information
#  (from libtool version 1.4.3 info page)

# GNU LibTool library interface numbers
#
# CURRENT
#     The most recent interface number that this library implements.
#
# REVISION
#     The implementation number of the CURRENT interface.
#
# AGE
#     The difference between the newest and oldest interfaces that
#     this library implements.  In other words, the library implements
#     all the interface numbers in the range from number `CURRENT -
#     AGE' to `CURRENT'.
#

#
#  1. Start with version information of `0:0:0' for each libtool
#     library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are
#     unnecessary, and only guarantee that the current interface
#     number gets larger faster.  We do it just after a release.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public
#     release, then set AGE to 0.
#
