# $Id: makefile,v 1.4 2001/07/15 21:18:53 rex Exp $
#
# Logon/login Application
#
# ReactOS Operating System
#
TARGET = winlogon
PROGS = winlogon.exe

PATH_TO_TOP = ../../..

BASE_CFLAGS = -I../../include

OBJECTS = winlogon.o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/lib/kernel32/kernel32.a \
       $(PATH_TO_TOP)/lib/ntdll/ntdll.a \
       $(PATH_TO_TOP)/lib/user32/user32.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) *.o $(TARGET).exe $(TARGET).sym $(TARGET).coff

.phony: clean

$(TARGET).exe: $(OBJECTS)
	$(CC) \
		$(OBJECTS) \
		$(LIBS) \
		-o $(TARGET).exe 
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym

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
