#
#
#
PATH_TO_TOP = ../..

CFLAGS = -g

OBJECTS= apc.o
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
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

apc.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o apc.exe
	$(NM) --numeric-sort apc.exe > apc.sym

include ../../rules.mak
