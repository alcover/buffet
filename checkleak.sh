#!/usr/bin/env sh

# check memory leaks of ./bin/arg
# ex: check src/example.c
# $ ./checkleak example

valgrind --leak-check=full ./bin/$1