CC = gcc
STD = c11
OPTIM = -O2
WARN = -Wall -Wextra -Wno-unused-function -Wno-unused-variable 
CP = $(CC) -std=$(STD) $(OPTIM) $(WARN) -g
LINK = $(CP) $^ -o $@

$(shell mkdir -p bin)

lib = bin/buffet
check = bin/check
example = bin/example

all: $(lib) $(check) $(example)
	
$(lib): src/buffet.c src/buffet.h
	@ echo $@
	@ $(CP) -c $< -o $@

$(check): src/check.c $(lib)
	@ echo $@
	@ $(CP) $< $(lib) -o $@

$(example): src/example.c $(lib)
	@ echo $@
	@ $(LINK)

check:
	@ ./$(check)

clean:
	@ rm -f bin/*

.PHONY: all check clean