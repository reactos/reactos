#
#
#
PATH_TO_TOP = ../..

OBJECTS = dump_shared_data.o
PROGS = dump_shared_data.exe
LIBS = 
CLEAN_FILES = dump_shared_data.o dump_shared_data.exe

all: dump_shared_data.exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

floppy: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

dump_shared_data.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o dump_shared_data.exe

BASE_CFLAGS = -I../../include
include ../../rules.mak
