# $Id: makefile,v 1.18 2001/07/25 06:19:34 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = blue

CFLAGS = -D__NTDRIVER__

OBJECTS = $(TARGET).o $(TARGET).coff 

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) *.o *.sym $(TARGET).coff junk.tmp base.tmp temp.exp blue.sys blue.sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

WARNINGS_ARE_ERRORS = yes
include $(PATH_TO_TOP)/rules.mak
