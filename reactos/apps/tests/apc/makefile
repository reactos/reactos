#
#
#
PATH_TO_TOP = ../..

OBJECTS= ../common/crt0.o apc.o
PROGS= apc.exe

BASE_CFLAGS = -I../../include
LIBS = ../../lib/crtdll/crtdll.a ../../lib/kernel32/kernel32.a \
       ../../lib/ntdll/ntdll.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) apc.o
	- $(RM) apc.exe
	- $(RM) apc.sym

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

apc.exe: $(OBJECTS) $(LIBS)
	$(LD) $(OBJECTS) $(LIBS) -o apc.exe
	$(NM) --numeric-sort apc.exe > apc.sym

include ../../rules.mak
