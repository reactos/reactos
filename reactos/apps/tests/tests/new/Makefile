#
#
#
PATH_TO_TOP = ../../..

PROGS = new

OBJECTS = new.o

LIBS = ../../../dk/w32/lib/gdi32.a 
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

new.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o new.exe
	$(NM) --numeric-sort new.exe > new.sym

include ../../../rules.mak
