# $Id: Makefile,v 1.1 2000/12/10 23:42:01 dwelch Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = minixfs

BASE_CFLAGS = -I../../../include

OBJECTS = block.o rw.o inode.o dir.o mount.o blockdev.o cache.o bitops.o \
          $(TARGET).coff ../../../ntoskrnl/ntoskrnl.a

all: $(TARGET).sys $(TARGET).sys.unstripped

.phony: all

clean:
	- $(RM) $(TARGET).o
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

$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS)

WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
