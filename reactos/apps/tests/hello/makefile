#
#
#
PATH_TO_TOP = ../..

OBJECTS = hello.o
PROGS = hello.exe
LIBS = 
CLEAN_FILES = hello.o hello.exe

all: hello.exe

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

hello.exe: $(OBJECTS)
	$(CC) $(OBJECTS) -o hello.exe

include ../../rules.mak
