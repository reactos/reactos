# $Id: makefile,v 1.1 2001/06/18 03:10:00 phreak Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=ramdrv

OBJECTS = ramdrv.o ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -I.

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
