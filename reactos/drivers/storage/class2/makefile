# $Id: makefile,v 1.1 2001/07/23 06:12:07 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=class2

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

$(TARGET).a: $(TARGET).def
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a


WARNINGS_ARE_ERRORS = yes
include $(PATH_TO_TOP)/rules.mak
