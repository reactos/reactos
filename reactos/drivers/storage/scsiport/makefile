# $Id: makefile,v 1.1 2001/07/21 07:30:26 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=scsiport

OBJECTS = $(TARGET).o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I$(PATH_TO_TOP)/include

all: $(TARGET).sys $(TARGET).sys.unstripped $(TARGET).a

.phony: all

clean:
	- $(RM) *.o $(TARGET).a $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped

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
