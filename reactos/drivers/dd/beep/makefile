# $Id: makefile,v 1.12 2001/07/25 06:19:24 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = beep

CFLAGS = -D__NTDRIVER__

OBJECTS = beep.o beep.coff 

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.PHONY: all

clean:
	- $(RM) *.o *.sym beep.coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

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
