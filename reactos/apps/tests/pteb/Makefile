#
#
#
PATH_TO_TOP = ../..

OBJECTS = pteb.o
PROGS = pteb.exe
LIBS = 
CLEAN_FILES = pteb.o pteb.exe

all: pteb.exe

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

pteb.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o pteb.exe

include ../../rules.mak
