#
#
#
OBJECTS= args.o
PROGS= args.exe

all: $(PROGS)

.phony: all

clean:
	- $(RM) args.o
	- $(RM) args.exe
	- $(RM) args.sym

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

args.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
