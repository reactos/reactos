# $Id: makefile,v 1.2 2001/07/15 21:18:53 rex Exp $
#
# Services: Service control manager (SCM)
#
# ReactOS Operating System
#
TARGET = services
PROGS = services.exe

PATH_TO_TOP = ../../..

OBJECTS = services.o services.coff
LIBS = ../../../lib/kernel32/kernel32.a ../../../lib/ntdll/ntdll.a
CLEAN_FILES = services.o services.coff services.exe services.sym

BASE_CFLAGS = -I../../../include

all: services.exe

$(TARGET).coff: $(TARGET).rc
	$(RC) $(TARGET).rc $(TARGET).coff

$(TARGET).exe: $(OBJECTS) $(LIBS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET).exe
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym

clean: 
	- $(RM) $(CLEAN_FILES)

.phony: clean 

install: $(PROGS:%=$(FLOPPY_DIR)/apps/%)

$(PROGS:%=$(FLOPPY_DIR)/apps/%): $(FLOPPY_DIR)/apps/%: %
	$(CP) $* $(FLOPPY_DIR)/apps/$*

dist: $(PROGS:%=../../../$(DIST_DIR)/apps/%)

$(PROGS:%=../../../$(DIST_DIR)/apps/%): ../../../$(DIST_DIR)/apps/%: %
	$(CP) $* ../../../$(DIST_DIR)/apps/$*


include $(PATH_TO_TOP)/rules.mak
