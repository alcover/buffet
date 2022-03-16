CC = gcc
OPTIM = -Og
WARN = -Wall -Wextra -Wno-unused-function -Wno-unused-variable 
CP = $(CC) -std=c11 $(WARN) -g
LINK = $(CP) $(OPTIM) $^ -o $@

$(shell mkdir -p bin)

lib = bin/buffet
asm = bin/buffet.asm
check = bin/check
example = bin/example
try = bin/try

all: $(lib) $(check) $(example) $(try) #$(asm)
	
$(lib): src/buffet.c src/buffet.h
	@ echo $@
	$(CP) $(OPTIM) -c $< -o $@

$(asm): $(lib)
	@ echo $@
# 	@ gcc ../src/buffet.c -S -O2 -o $@
	@ objdump -M intel --no-show-raw-insn -d -S $< > $@ 

$(check): src/check.c $(lib)
	@ echo $@
	@ $(CP) -Og $< $(lib) -o $@
	@ ./$@

$(example): src/example.c $(lib)
	@ echo $@
	@ $(LINK) 

$(try): src/try.c $(lib)
	@ echo $@
	@ $(LINK)

check:
	@ ./$(check)

clean:
	@ rm -f bin/*

.PHONY: all check clean