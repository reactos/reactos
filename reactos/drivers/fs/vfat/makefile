# $Id: makefile,v 1.31 2001/06/22 12:27:48 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=vfatfs

OBJECTS = blockdev.o close.o create.o dir.o dirwr.o iface.o string.o fat.o \
          rw.o finfo.o volume.o shutdown.o cleanup.o fcb.o $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -g -Wall -Werror

all: $(TARGET).sys.unstripped $(TARGET).sys

.phony: all

clean:
ifeq ($(DOSCLI),yes)
	- $(RM) *.o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).sys
	- $(RM) $(TARGET).sys.unstripped
else
	- $(RM) *.o $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sys.unstripped
endif

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys


$(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)
	$(LD) -r -o vfat.o $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" vfat.o $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 -Wl,temp.exp -Wl,"-h vfatfs.sys" -specs=../../svc_specs -mdll -o $(TARGET).sys.unstripped vfat.o $(LIBS)
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).sys.unstripped >$(TARGET).sym


$(TARGET).sys: $(OBJECTS) $(LIBS)
	$(LD) -r -o vfat.o $(OBJECTS)
	$(STRIP) --strip-debug vfat.o
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" vfat.o $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 -Wl,temp.exp -Wl,"-h vfatfs.sys" -specs=../../svc_specs -mdll -o $(TARGET).sys vfat.o $(LIBS)
	- $(RM) temp.exp

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
