#
#
#
PATH_TO_TOP = ../..

OBJECTS= ../common/crt0.o regtest.o
PROGS= regtest.exe
LIBS= ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a \
      ../../lib/advapi32/advapi32.a

BASE_CFLAGS = -I../../include

all: $(PROGS)

.phony: all

clean:
	- $(RM) *.o
	- $(RM) *.exe
	- $(RM) *.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

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

regtest.exe: $(OBJECTS) $(LIBS)
	$(CC) -nostartfiles -nostdlib $(OBJECTS) $(LIBS) -lgcc -o regtest.exe
	$(NM) --numeric-sort regtest.exe > regtest.sym

include ../../rules.mak
