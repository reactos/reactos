#
#
#
PATH_TO_TOP = ../..

OBJECTS= args.o
PROGS= args.exe

BASE_CFLAGS = -I../../include

all: $(PROGS)

.phony: all

clean:
	- $(RM) args.o args.exe args.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

args.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o args.exe
	$(NM) --numeric-sort args.exe > args.sym

include ../../rules.mak
