#
#
#
OBJECTS= cat.o
PROGS= cat.exe

all: cat.exe

.phony: all

clean:
	- $(RM) cat.o
	- $(RM) cat.exe
	- $(RM) cat.sym

.phony: clean

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

.PHONY: floppy dist

cat.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o cat.exe
	$(NM) --numeric-sort cat.exe > args.sym

include ../../rules.mak
