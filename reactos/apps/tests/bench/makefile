#
#
#

all: bench-thread.exe

.phony: all

clean:
	- $(RM) test-stdio.o
	- $(RM) test-stdio.exe
	- $(RM) test-stdio.sym

.phony: clean

bench-thread.exe: bench-thread.c
	$(CC) bench-thread.c -lkernel32 -o bench-thread.exe
	$(NM) --numeric-sort bench-thread.exe > bench-thread.sym

include ../../rules.mak
