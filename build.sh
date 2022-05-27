#!/usr/bin/env sh
CC="cc"
LDFLAGS="-lzkcollection -lreadline"
OUT="nc"
CFILES="$(find src -name '*.c')"

echo "Compiling..."
$CC -Wall -Wextra $LDFLAGS -O2 -o $OUT $CFILES $*
rm -rf *.o
echo "Done"
