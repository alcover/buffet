CC = gcc
STD = c11
OPTIM = -O0
WARN = -Wall -Wextra -Wno-unused-function -Wno-unused-variable 
CP = $(CC) -std=$(STD) $(OPTIM) $(WARN) -g #-pg
COMP = $(CP) -c $< -o $@
LINK = $(CP) $^ -o $@

$(shell mkdir -p bin)

lib = bin/rocket
check = bin/check
test = bin/test

all: $(lib) $(test) $(check)
	
$(lib): src/buf.c src/buf.h #src/util.c
	@ echo $@
	@ $(COMP)

$(check): src/check.c $(lib) src/util.c
	@ echo $@
	@ $(CP) $< $(lib) -o $@

$(test): src/test.c $(lib) #util.c
	@ echo $@	
	@ $(CP) $< $(lib) -o $@

check:
	@ ./$(check)

clean:
	@ rm -f bin/* test

.PHONY: all check test clean