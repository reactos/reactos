#
#
#
PATH_TO_TOP = ../..

PROGS = bench-thread

all: $(PROGS:%=%.exe)

.phony: all

clean: $(PROGS:%=%_clean)

$(PROGS:%=%_clean): %_clean:
	- $(RM) $*.o
	- $(RM) $*.exe
	- $(RM) $*.sym

.phony: clean

install: # $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

$(PROGS:%=$(FLOPPY_DIR)/apps/%.exe): $(FLOPPY_DIR)/apps/%.exe: %.exe
ifeq ($(DOSCLI),yes)
	$(CP) $*.exe $(FLOPPY_DIR)\apps\$*.exe
else
	$(CP) $*.exe $(FLOPPY_DIR)/apps/$*.exe
endif

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%.exe)

$(PROGS:%=../../$(DIST_DIR)/apps/%.exe): ../../$(DIST_DIR)/apps/%.exe: %.exe
ifeq ($(DOSCLI),yes)
	$(CP) $*.exe ..\..\$(DIST_DIR)\apps\$*.exe
else
	$(CP) $*.exe ../../$(DIST_DIR)/apps/$*.exe
endif

bench-thread.exe: bench-thread.c
	$(CC) $(CFLAGS) bench-thread.c -lkernel32 -o bench-thread.exe
	$(NM) --numeric-sort bench-thread.exe > bench-thread.sym

include ../../rules.mak
