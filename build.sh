#!/usr/bin/env sh
CC="cc"
LDFLAGS="-I/opt/local/include -L/opt/local/lib -lzkcollection -lreadline"
OUT="nc"
CFILES="$(find src -name '*.c')"

echo "Compiling\n"
CC -Wall -Wextra -O2 -ggdb -c $CFILES
CC $LDFLAGS -o $OUT *.o

#rm -rf *.o
