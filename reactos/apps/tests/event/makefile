#
#
#
PROGS = event

all: $(PROGS:%=%.exe)

.phony: all

clean: $(PROGS:%=%_clean)

$(PROGS:%=%_clean): %_clean:
	- $(RM) $*.o
	- $(RM) $*.exe
	- $(RM) $*.sym

.phony: clean

floppy: # $(PROGS:%=$(FLOPPY_DIR)/apps/%.exe)

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

event.exe: event.c
	$(CC) $(CFLAGS) event.c -lkernel32 -o event.exe
	$(NM) --numeric-sort event.exe > event.sym

include ../../rules.mak
