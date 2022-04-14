#!/usr/bin/env sh

# check memory leaks of ./bin/arg
# ex: to check src/example.c do `./checkleak example`

valgrind --leak-check=full ./bin/$1