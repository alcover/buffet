#!/usr/bin/env sh

# check for memory leaks during unit-tests
valgrind --leak-check=full ./bin/check