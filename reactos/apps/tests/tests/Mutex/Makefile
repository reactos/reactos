# 10-11-01 Steven Edwards <Steven.Edwards@KnowledgeSentry.net>
# Mutex Benchmarks from the Wineserver Linux Kernel Module
#

PATH_TO_TOP = ../../..

PROGS =  fivemutex rapidmutex 

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.exe *.sym

.phony: clean

fivemutex.exe: fivemutex.c
	$(CC) fivemutex.c -lkernel32 -o fivemutex.exe
	$(NM) --numeric-sort fivemutex.exe > fivemutex.sym

rapidmutex.exe: rapidmutex.c
	$(CC) rapidmutex.c -lkernel32 -o rapidmutex.exe
	$(NM) --numeric-sort rapidmutex.exe > rapidmutex.sym

include ../../../rules.mak
