#!/usr/bin/env sh

# check memory leaks of ./bin/arg
# ex: to check src/example.c do `./checkleak example`

clear && valgrind --leak-check=full --show-leak-kinds=all ./bin/$1