#
#
#
PATH_TO_TOP = ../../../..

PROGS = Parent_Child

OBJECTS = Parent_Child.o

LIBS = $(PATH_TO_TOP)/dk/w32/lib/gdi32.a 
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

Parent_Child.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o Parent_Child.exe
	$(NM) --numeric-sort Parent_Child.exe > Parent_Child.sym

include $(PATH_TO_TOP)/rules.mak
