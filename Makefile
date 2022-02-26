CC = gcc
STD = c11
OPTIM = -O2
WARN = -Wall -Wextra -Wno-unused-function -Wno-unused-variable 
CP = $(CC) -std=$(STD) $(OPTIM) $(WARN) -g
LINK = $(CP) $^ -o $@

$(shell mkdir -p bin)

lib = bin/buffet
check = bin/check

all: $(lib) $(check)
	
$(lib): src/buffet.c src/buffet.h
	@ echo $@
	@ $(CP) -c $< -o $@

$(check): src/check.c $(lib)
	@ echo $@
	@ $(CP) $< $(lib) -o $@

check:
	@ ./$(check)

clean:
	@ rm -f bin/*

.PHONY: all check clean