# $Id: Makefile,v 1.3 2001/06/12 12:33:10 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=minixfs

OBJECTS = $(TARGET).o $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -I. -g

all: $(TARGET).sys

.phony: all

clean:
	- $(RM) *.o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys
	- $(RM) $(TARGET).sys.unstripped

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

MOBJECTS = block.o blockdev.o cache.o dir.o inode.o mount.o rw.o bitops.o

minixfs.o: $(MOBJECTS)
	$(LD) -r -o minixfs.o $(MOBJECTS)

$(TARGET).sys: $(OBJECTS)

WITH_DEBUGGING=yes
include ../../../rules.mak
