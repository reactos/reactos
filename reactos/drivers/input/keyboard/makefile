# $Id: makefile,v 1.4 2001/06/22 12:28:00 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=keyboard

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -O2 -I. -g

all: $(TARGET).sys $(TARGET).sys.unstripped

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

WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
