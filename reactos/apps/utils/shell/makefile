#
#
#
OBJECTS= ../common/crt0.o shell.o
PROGS= shell.exe
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a
CLEAN_FILES= shell.o shell.exe shell.sym

all: shell.exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

floppy: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* $(FLOPPY_DIR)\apps\$*
else
	$(CP) $* $(FLOPPY_DIR)/apps/$*
endif

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
ifeq ($(DOSCLI),yes)
	$(CP) $* ..\..\$(DIST_DIR)\apps\$*
else
	$(CP) $* ../../$(DIST_DIR)/apps/$*
endif

shell.exe: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../specs $(OBJECTS) $(LIBS) -lgcc -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym

include ../../rules.mak
