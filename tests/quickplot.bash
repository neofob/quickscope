#!/bin/bash

if [ -z "$1" ] ; then
  echo "Usage: $0 PROG [ARGS ...]"
  echo
  echo "Runs: \$* | quickplot -"
  exit 1
fi

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit 1
cd $scriptdir || exit 1

$* | quickplot -P
