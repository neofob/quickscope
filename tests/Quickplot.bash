
if [ -z "$1" ] ; then
  echo "Usage: $0 PROG [ARGS ...]"
  echo
  echo "Runs: \$* | quickplot -"
  exit 1
fi

if ! which quickplot > /dev/null ; then
  echo "You need quickplot installed to run this."
  echo "Try running:"
  echo "  sudo apt-get install quickplot"
  echo "or something like that."
  exit 1
fi

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit 1
cd $scriptdir || exit 1

echo "fuck"

./$* | quickplot -
