#
#
#
PATH_TO_TOP = ../../..

PROGS = rolex

OBJECTS = rolex.o

LIBS = ../../../dk/w32/lib/gdi32.a 
CFLAGS =

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

rolex.exe: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,--subsystem,windows $(OBJECTS) $(LIBS) -o rolex.exe
	$(NM) --numeric-sort rolex.exe > rolex.sym

#%.coff: %.rc
#	$(RC) $(RCINC) $< -o $@

include ../../../rules.mak
