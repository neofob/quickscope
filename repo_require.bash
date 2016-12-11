#! /bin/bash

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

# This file is just sourced and does not need to be executable.

function FAIL()
{
    echo -e "$0 failed in file: ${BASH_SOURCE[0]} \
line: ${BASH_LINENO[0]}"
    echo
    [ -n "$1" ] &&\
      echo "Run: $0 --help to see script help"
    exit 1
}

function check_repo_require()
{

  # Building from repo required programs with friendly
  # what, where to get.
  local req_bin[0]="convert"
  local req_desc[0]="\
  You need the 'convert' program from the ImageMagick package\n\
  homepage: http://www.imagemagick.org/\n\
  debian package: apt-get install imagemagick\n"

  local req_bin[1]="autoreconf"
  local req_desc[1]="\
  You need the 'autoreconf' program from the autoconf package\n\
  homepage: http://www.gnu.org/software/autoconf/\n\
  debian package: apt-get install autoconf\n"

  local req_bin[2]="libtoolize"
  local req_desc[2]="\
  You need the 'libtoolize' program from the libtool package\n\
  homepage: http://www.gnu.org/software/libtool/\n\
  debian package: apt-get install libtool\n"



  local fail=
  if [ ! -d ".git" ] ; then
    cat <<END
  The .git repository directory was not found.  So
  it does not look like you need to, or should, run this.

END
    fail=yes
  fi

  local i=0
  local need=
  while [ -n "${req_bin[$i]}" ] ; do
    if ! which ${req_bin[$i]} > /dev/null ; then
      need="$need $i"
      fail=yes
    fi
    let i=$i+1
  done

  if [ -n "$fail" ] ; then
    for i in $need ; do
      echo "  Program ${req_bin[$i]} was not found in your path"
      echo -e "${req_desc[$i]}"
      echo
    done
    echo "ERROR: Consider the above:"
    echo
    FAIL --help
  fi
}

