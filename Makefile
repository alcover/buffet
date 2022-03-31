CC = gcc
OPTIM = -Og
WARN = -Wall -Wextra
CP = $(CC) -std=c11 $(WARN) -g
CPP = g++ -std=c++17 $(WARN) -fpermissive -g
LINK = $(CP) $(OPTIM) $^ -o $@

$(shell mkdir -p bin)

lib =		bin/buffet
asm =		bin/buffet.asm
check =		bin/check
benchcpp =		bin/benchcpp
utilcpp = 	bin/utilcpp
example =	bin/example
examples =	bin/examples
try =		bin/try

all: $(lib) $(check) $(example) $(examples) #$(benchcpp)
	
$(lib): src/buffet.c src/buffet.h
	@ echo make $@
	@ $(CP) $(OPTIM) -c $< -o $@

$(asm): $(lib)
	@ echo make $@
	@ objdump -M intel --no-show-raw-insn -d -S $< > $@ 

$(check): src/check.c $(lib)
	@ echo make $@
	@ $(CP) -Og $^ -o $@
	@ ./$@

# requires libbenchmark-dev
$(benchcpp): src/bench.cpp $(lib) $(utilcpp)
	@ echo make $@
	@ $(CPP) $(OPTIM) -w -lbenchmark -lpthread $^ -o $@

$(utilcpp): src/utilcpp.cpp src/utilcpp.h
	@ echo make $@
	@ $(CPP) $(OPTIM) -c $< -o $@

bin/%: src/%.c $(lib)
	@ echo make $@
	@ $(LINK)

check:
	@ ./$(check)

benchcpp: 
	@ ./$(benchcpp) --benchmark_color=false

clean:
	@ rm -f bin/*

.PHONY: all check benchcpp clean try