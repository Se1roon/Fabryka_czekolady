#!/bin/bash

CFLAGS="-Wall -Wextra -g"

clang -o ./bin/fabryka ./src/fabryka.c $CFLAGS
clang -o ./bin/dostawcy ./src/dostawcy.c $CFLAGS
clang -o ./bin/dyrektor ./src/dyrektor.c $CFLAGS

./bin/dyrektor
