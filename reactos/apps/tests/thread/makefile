#
#
#
PATH_TO_TOP = ../..

PROGS = thread

all: $(PROGS:%=%.exe)

.phony: all

clean: 
	- $(RM) *.o *.exe *.sym

.phony: clean

install: # $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe

thread.exe: thread.c
	$(CC) $(CFLAGS) thread.c -lkernel32 -o thread.exe
	$(NM) --numeric-sort thread.exe > thread.sym

include ../../rules.mak
