# $Id: Makefile,v 1.4 2000/10/07 18:44:07 dwelch Exp $
#
#
PATH_TO_TOP = ../../../..

CFLAGS = -Iinclude

TARGETNAME = ne2000

OBJECTS = ne2000/main.o ne2000/8390.o $(TARGET).coff $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

RESOURCE_OBJECT  = $(TARGETNAME).coff
NE2000_OBJECTS   = ne2000/main.o ne2000/8390.o $(RESOURCE_OBJECT)
IMPORT_LIBS      = ../../ndis/ndis.a ../../../../ntoskrnl/ntoskrnl.a


all: objects $(TARGETNAME).sys

objects:
	mkdir objects

objects/ne2000.o: $(NE2000_OBJECTS)
	$(LD) -r $(NE2000_OBJECTS) -o objects/ne2000.o

OBJECTS = objects/ne2000.o

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = \
		*.o ne2000\*.o objects\*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = \
		*.o ne2000\*.o objects/*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
endif

clean:

$(TARGETNAME).sys: $(OBJECTS) $(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def $(TARGETNAME).def \
		--output-lib $(TARGETNAME).a \
		--kill-at
	$(CC) \
        -specs=../../../../specs \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DriverEntry@8 \
		-o junk.tmp \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
        $(TARGETNAME).o \
        $(IMPORT_LIBS)
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).sys: $(OBJECTS)

WITH_DEBUGGING=yes
include ../../../../rules.mak

