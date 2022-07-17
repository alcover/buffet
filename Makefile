ifdef DEBUG
	DEBUG =
else
	DEBUG = -DNDEBUG
endif

ifdef MEMCHECK
	MEMCHECK = -DMEMCHECK
$(info MEMCHECK enabled)
endif

CC = gcc
OPTIM = -O2
WARN = -Wall -Wextra -Wno-unused-function
CP = $(CC) -std=c11 $(WARN) -g
CPP = g++ -std=c++2a -fpermissive -g
LINK = $(CP) $(OPTIM) $^ -o $@

$(shell mkdir -p bin/ex)

lib = bin/libbuffet.a
asm = bin/libbuffet.s
check = bin/check
bench =	bin/bench
ex := $(patsubst src/ex/%.c,bin/ex/%,$(wildcard src/ex/*.c))

all: $(lib) $(check) $(ex) $(bench) README.md #$(asm) bin/threadtest

$(lib): src/buffet.c src/buffet.h
	@ echo make $@
	@ $(CP) $(DEBUG) $(MEMCHECK) $(OPTIM) -c $< -o $@

OBJDUMP := $(shell objdump -v 2>/dev/null)

$(asm): $(lib)
	@ echo make $@
ifdef OBJDUMP 
	@ objdump -d -Mintel --no-show-raw-insn $< > $@ 
else
	@ echo objdump not installed
endif

$(check): src/check.c $(lib)
	@ echo make $@
	@ $(CP) $(MEMCHECK) -O0 $^ -o $@ -Wno-unused-function
	@ ./$@

LIBBENCHMARK := $(shell /sbin/ldconfig -p | grep libbenchmark 2>/dev/null)

# requires libbenchmark-dev
$(bench): src/bench.cpp $(lib) bin/utilcpp
	@ echo make $@
ifdef LIBBENCHMARK
	@ $(CPP) $(OPTIM) -o $@ $^ -lbenchmark -lpthread
else
	@ echo libbenchmark not installed
endif

bin/utilcpp: src/utilcpp.cpp src/utilcpp.h
	@ echo make $@
	@ $(CPP) $(OPTIM) -c $< -o $@

bin/ex/%: src/ex/%.c $(lib)
	@ echo make $@
	@ $(LINK)

bin/threadtest: src/threadtest.c $(lib)
	@ echo make $@
	@ $(LINK) -lpthread

README.md: src/README.tpl.md src/ex/*
	@ echo make $@
	@ sed -e '/<schema.c>/{r src/ex/schema.c' -e 'd}' \
		  -e '/<view.c>/{r src/ex/view.c' -e 'd}' \
		  -e '/<free.c>/{r src/ex/free.c' -e 'd}' \
		  src/README.tpl.md > $@

check:
	@ ./$(check)

bench: 
	@ ./$(bench) --benchmark_color=false --benchmark_format=console

clean:
	@ rm -rf bin/*

.PHONY: all check bench clean