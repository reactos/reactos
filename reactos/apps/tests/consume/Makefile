#
#
#
PATH_TO_TOP = ../..

OBJECTS = consume.o
PROGS = consume.exe
LIBS = 
CLEAN_FILES = consume.o consume.exe

all: consume.exe

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

consume.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o consume.exe

include ../../rules.mak
