#
#
#
PATH_TO_TOP = ../..

OBJECTS= cat.o
PROGS= cat.exe

all: cat.exe

.phony: all

clean:
	- $(RM) cat.o
	- $(RM) cat.exe
	- $(RM) cat.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

.PHONY: floppy dist

cat.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o cat.exe
	$(NM) --numeric-sort cat.exe > cat.sym

include ../../rules.mak
