# $Id: makefile,v 1.2 2001/06/22 12:25:24 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=ramdrv

OBJECTS = ramdrv.o ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I.

all: $(TARGET).sys

ramdrv.o: ramdrv.h

.phony: all

clean:
	- $(RM) floppy.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys
	- $(RM) $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys: $(OBJECTS)

WITH_DEBUGGING=yes
include ../../../rules.mak
