# $Id: makefile,v 1.6 2001/07/24 18:37:46 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = keyboard

OBJECTS = $(TARGET).o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -O2 -I. -g

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) *.o *.sym $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

$(TARGET).coff: $(PATH_TO_TOP)/include/reactos/buildno.h $(TARGET).rc

WARNINGS_ARE_ERRORS = yes
include $(PATH_TO_TOP)/rules.mak
