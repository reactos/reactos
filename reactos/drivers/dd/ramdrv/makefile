# $Id: makefile,v 1.4 2001/07/25 06:20:17 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = ramdrv

OBJECTS = ramdrv.o

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I.

all: $(TARGET).sys

ramdrv.o: ramdrv.h

.phony: all

clean:
	- $(RM) *.o *.sym *.coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

WITH_DEBUGGING=yes
include $(PATH_TO_TOP)/rules.mak
