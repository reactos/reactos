# $Id: Makefile,v 1.7 2001/07/24 16:30:36 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = minixfs

OBJECTS = $(TARGET).o minix.coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I. -g

all: $(TARGET).sys

.phony: all

clean:
	- $(RM) *.o *.coff junk.tmp base.tmp temp.exp *.sym $(TARGET).sys $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

MOBJECTS = block.o blockdev.o cache.o dir.o inode.o mount.o rw.o bitops.o

minixfs.o: $(MOBJECTS)
	$(LD) -r -o minixfs.o $(MOBJECTS)

$(TARGET).sys: $(OBJECTS) minix.coff

minix.coff: $(PATH_TO_TOP)/include/reactos/buildno.h minix.rc

WITH_DEBUGGING=yes
include $(PATH_TO_TOP)/rules.mak
