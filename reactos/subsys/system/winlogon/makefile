# $Id: makefile,v 1.1 2000/12/05 02:38:08 ekohl Exp $
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

LIBS = ../../../lib/kernel32/kernel32.a ../../../lib/ntdll/ntdll.a

all: $(TARGET).exe

.phony: all

clean:
	- $(RM) *.o
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

.phony: clean

$(TARGET).coff: $(TARGET).rc
	$(RC) $(TARGET).rc $(TARGET).coff

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
