#
#
#
PATH_TO_TOP = ../..

PROGS = bench-thread

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

bench-thread.exe: bench-thread.c
	$(CC) $(CFLAGS) bench-thread.c -lkernel32 -o bench-thread.exe
	$(NM) --numeric-sort bench-thread.exe > bench-thread.sym

include ../../rules.mak
