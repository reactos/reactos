# $Id: makefile,v 1.14 2001/07/25 08:28:52 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = serial

OBJECTS = $(TARGET).o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) *.o *.sym $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

include $(PATH_TO_TOP)/rules.mak
