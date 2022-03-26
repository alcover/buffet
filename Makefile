CC = gcc
OPTIM = -Og
WARN = -Wall -Wextra
CP = $(CC) -std=c11 $(WARN) -g
LINK = $(CP) $(OPTIM) $^ -o $@

$(shell mkdir -p bin)

lib =		bin/buffet
asm =		bin/buffet.asm
check =		bin/check
benchcpp =	bin/benchcpp
benchbook =	bin/benchbook
benchbookcpp =	bin/benchbookcpp
example =	bin/example
try =		bin/try

all: $(lib) $(check) $(example) #$(benchcpp) #$(benchbook) #$(benchbookcpp)
	
$(lib): src/buffet.c src/buffet.h
	@ echo make $@
	$(CP) $(OPTIM) -c $< -o $@

$(asm): $(lib)
	@ echo make $@
	@ objdump -M intel --no-show-raw-insn -d -S $< > $@ 

$(check): src/check.c $(lib)
	@ echo make $@
	@ $(CP) -Og $^ -o $@
	@ ./$@

# requires libbenchmark-dev 
$(benchcpp): src/bench.cpp $(lib)
	@ echo make $@
	@ g++ -std=c++2a $(OPTIM) -w -fpermissive -lbenchmark -lpthread $^ -o $@

$(benchbook): src/benchbook.cpp $(lib)
	@ echo make $@
	@ g++ $(OPTIM) $^ -o $@


$(example): src/example.c $(lib)
	@ echo make $@
	@ $(LINK) 

$(try): src/try.c $(lib)
	@ echo make $@
	@ $(LINK) -Wno-unused-function -Wno-unused-variable 

check:
	@ ./$(check)

try: $(try)
	@ ./$(try)

clean:
	@ rm -f bin/*

.PHONY: all check clean try