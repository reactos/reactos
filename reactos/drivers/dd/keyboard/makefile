# $Id: makefile,v 1.11 2000/08/18 22:27:05 dwelch Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=keyboard

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I. -I../../../include

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) $(TARGET).o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys $(FLOPPY_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys
endif

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys ..\..\..\$(DIST_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys
endif

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)

include ../../../rules.mak
