#
#
#
PATH_TO_TOP = ../..

PROGS = winhello

OBJECTS = 
LIBS= ../../lib/kernel32/kernel32.a ../../lib/gdi32/gdi32.a
CFLAGS = -I../../include

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.exe *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

winhello.exe: winhello.c
	$(CC) $(CFLAGS) winhello.c $(LIBS) -o winhello.exe
	$(NM) --numeric-sort winhello.exe > winhello.sym

include ../../rules.mak
