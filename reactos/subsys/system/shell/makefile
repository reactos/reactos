# $Id: makefile,v 1.2 2001/02/12 03:50:00 phreak Exp $
#
# Shell: Simple shell application
#
# ReactOS Operating System
#
TARGET = shell
PROGS = shell.exe

PATH_TO_TOP = ../../..

OBJECTS= shell.o shell.coff
LIBS= ../../../lib/kernel32/kernel32.a ../../../lib/ntdll/ntdll.a
CLEAN_FILES= shell.o shell.coff shell.exe shell.sym

BASE_CFLAGS = -I../../../include

all: shell.exe

shell.exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -lgcc -o shell.exe
	$(NM) --numeric-sort shell.exe > shell.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../../$(DIST_DIR)/apps/%): ../../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../../$(DIST_DIR)/apps/$*


include $(PATH_TO_TOP)/rules.mak
