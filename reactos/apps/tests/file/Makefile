#
#
#
PATH_TO_TOP = ../..

OBJECTS= file.o
PROGS= file.exe

BASE_CFLAGS = -I../../include

all: $(PROGS)

.phony: all

clean:
	- $(RM) file.o
	- $(RM) file.exe
	- $(RM) file.sym

.phony: clean

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../$(DIST_DIR)/apps/%): ../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../$(DIST_DIR)/apps/$*

file.exe: $(OBJECTS)
	$(CC) $(OBJECTS) $(BASE_CFLAGS) -o file.exe
	$(NM) --numeric-sort file.exe > file.sym

include ../../rules.mak









