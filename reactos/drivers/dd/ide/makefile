# $Id: makefile,v 1.15 2000/08/18 22:27:04 dwelch Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=ide

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I. -I../../../include

all: $(TARGET).sys.unstripped $(TARGET).sys

.phony: all

clean:
	- $(RM) $(TARGET).o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys
	- $(RM) $(TARGET).nostrip.sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)

WITH_DEBUGGING=yes
include ../../../rules.mak
