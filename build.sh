#!/bin/bash

CFLAGS="-Wall -Wextra"

clang -o ./bin/logging ./src/logging.c $CFLAGS
clang -o ./bin/fabryka ./src/fabryka.c $CFLAGS
clang -o ./bin/dostawcy ./src/dostawcy.c $CFLAGS
clang -o ./bin/dyrektor ./src/dyrektor.c $CFLAGS

./bin/dyrektor
