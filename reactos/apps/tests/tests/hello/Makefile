#
#
#
PATH_TO_TOP = ../../../..

PROGS = hello

OBJECTS = hello.o

LIBS = $(PATH_TO_TOP)/dk/w32/lib/gdi32.a 
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

hello.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o hello.exe
	$(NM) --numeric-sort hello.exe > hello.sym

include $(PATH_TO_TOP)/rules.mak
