# $Id: makefile,v 1.3 2001/07/15 21:18:58 rex Exp $
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
	- $(RM) floppy.o junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

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
