# $Id: makefile,v 1.10 2001/06/22 12:23:49 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = beep

CFLAGS = -D__NTDRIVER__ -I../../../include

OBJECTS = beep.o beep.coff ../../../ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.PHONY: all

clean:
	- $(RM) beep.o
	- $(RM) beep.coff
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
