# $Id: makefile,v 1.21 2001/02/18 22:16:05 dwelch Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=ide

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -I. -g

all: $(TARGET).sys.unstripped $(TARGET).sys

.phony: all

clean:
	- $(RM) $(TARGET).o
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

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
