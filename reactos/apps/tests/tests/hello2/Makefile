#
#
#
PATH_TO_TOP = ../../..

PROGS = hello2

OBJECTS = hello2.o

LIBS = # gdi32.a 
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

hello2.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o hello2.exe
	$(NM) --numeric-sort hello2.exe > hello2.sym

include ../../../rules.mak
