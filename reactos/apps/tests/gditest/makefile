#
#
#
PATH_TO_TOP = ../..

PROGS = gditest

OBJECTS = 
LIBS= ../../lib/kernel32/kernel32.a ../../lib/gdi32/gdi32.a
CFLAGS = -I../../include

all: $(PROGS:%=%.exe)

.phony: all

$(PROGS:%=%_clean): %_clean:
	- $(RM) $*.o
	- $(RM) $*.exe
	- $(RM) $*.sym

clean: $(PROGS:%=%_clean)

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

gditest.exe: gditest.c
	$(CC) gditest.c $(LIBS) -lgcc -o gditest.exe
	$(NM) --numeric-sort gditest.exe > gditest.sym

include ../../rules.mak
