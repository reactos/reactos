#
#
#
PATH_TO_TOP = ../..

OBJECTS = atomtest.o
PROGS = atomtest.exe
LIBS = ../../lib/crtdll/crtdll.a ../../lib/kernel32/kernel32.a \
       ../../lib/ntdll/ntdll.a

CLEAN_FILES = atomtest.o atomtest.exe

all: atomtest.exe

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

atomtest.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o atomtest.exe
	$(NM) --numeric-sort atomtest.exe > atomtest.sym

include ../../rules.mak
