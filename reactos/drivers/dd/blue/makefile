# $Id: makefile,v 1.13 2000/08/18 22:27:04 dwelch Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = blue

BASE_CFLAGS = -I../../../include

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) $(TARGET).o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) blue.sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)

include ../../../rules.mak
