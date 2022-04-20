CC = gcc
OPTIM = -O2
WARN = -Wall -Wextra
CP = $(CC) -std=c11 $(WARN) -g
CPP = g++ -std=c++2a $(WARN) -fpermissive -g
LINK = $(CP) $(OPTIM) $^ -o $@

$(shell mkdir -p bin)
$(shell mkdir -p bin/ex)

OBJDUMP := $(shell objdump -v 2>/dev/null)

lib =		bin/buffet
asm =		bin/buffet.s
check =		bin/check
benchcpp =	bin/benchcpp
examples := $(patsubst src/ex/%.c,bin/ex/%,$(wildcard src/ex/*.c))

all: $(lib) $(asm) $(check) $(examples) bin/try #$(benchcpp)

$(lib): src/buffet.c src/buffet.h
	@ echo make $@
	@ $(CP) $(OPTIM) -c $< -o $@

$(asm): $(lib)
	@ echo make $@
ifdef OBJDUMP 
	@ objdump -d -Mintel --no-show-raw-insn $< > $@ 
else
	@ echo objdump not installed
endif

$(check): src/check.c $(lib)
	@ echo make $@
	@ $(CP) -O0 $^ -o $@
	@ ./$@

# requires libbenchmark-dev
$(benchcpp): src/bench.cpp $(lib) bin/utilcpp
	@ echo make $@
	@ $(CPP) -O2 -w -lbenchmark -lpthread $^ -o $@

bin/utilcpp: src/utilcpp.cpp src/utilcpp.h
	@ echo make $@
	@ $(CPP) $(OPTIM) -c $< -o $@

bin/ex/%: src/ex/%.c $(lib)
	@ echo make $@
	@ $(LINK)

bin/%: src/%.c $(lib)
	@ echo make $@
	@ $(LINK)

check:
	@ ./$(check)

benchcpp: 
	@ ./$(benchcpp) --benchmark_color=false

clean:
	@ rm -rf bin/*

.PHONY: all check benchcpp clean