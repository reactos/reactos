# $Id: Makefile,v 1.7 2001/06/22 12:24:04 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=floppy

OBJECTS = floppy.o isr.o dpc.o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I.

all: $(TARGET).sys

floppy.o: floppy.h

isr.o: floppy.h

dpc.o: floppy.h

.phony: all

clean:
	- $(RM) floppy.o
	- $(RM) isr.o
	- $(RM) dpc.o
	- $(RM) $(TARGET).coff
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
