# $Id: Makefile,v 1.11 2001/07/29 16:40:20 ekohl Exp $
#
# Named Pipes File System
#
PATH_TO_TOP=../../..

TARGET = npfs

CFLAGS = -D__NTDRIVER__

OBJECTS = create.o finfo.o fsctrl.o npfs.o rw.o volume.o npfs.coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

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

npfs.coff: $(PATH_TO_TOP)/include/reactos/buildno.h npfs.rc

include $(PATH_TO_TOP)/rules.mak
