#
#
#
PATH_TO_TOP = ../../../..

PROGS = guitest

OBJECTS = guitest.o 

LIBS = $(PATH_TO_TOP)/dk/w32/lib/gdi32.a 
CFLAGS = -Wall -Werror

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.exe *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/tests/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/tests/%.exe): $(FLOPPY_DIR)/apps/tests/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

guitest.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o guitest.exe
	$(NM) --numeric-sort guitest.exe > guitest.sym

include $(PATH_TO_TOP)/rules.mak
