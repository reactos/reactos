# $Id: makefile,v 1.1 2001/07/24 10:21:15 ekohl Exp $
#
# disk.sys makefile
#
PATH_TO_TOP = ../../..

TARGET=disk

OBJECTS = $(TARGET).o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a ../class2/class2.a

CFLAGS = -D__NTDRIVER__ -I.

all: $(TARGET).sys.unstripped $(TARGET).sys

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

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes

include $(PATH_TO_TOP)/rules.mak

#EOF
