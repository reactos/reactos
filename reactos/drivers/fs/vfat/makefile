# $Id: makefile,v 1.28 2001/03/07 13:44:41 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET=vfatfs

OBJECTS = blockdev.o close.o create.o dir.o dirwr.o iface.o string.o fat.o \
          rw.o finfo.o volume.o shutdown.o cleanup.o $(TARGET).coff
LIBS = ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -g -Wall -Werror

all: $(TARGET).nostrip.sys $(TARGET).sys

.phony: all

clean:
ifeq ($(DOSCLI),yes)
	- $(RM) *.o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys
else
	- $(RM) $(OBJECTS) $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).nostrip.sys
endif

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys

$(TARGET).nostrip.sys: $(OBJECTS) $(LIBS)
	$(LD) -r -o vfat.o $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" vfat.o $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 -Wl,temp.exp -Wl,"-h vfatfs.sys" -specs=../../svc_specs -mdll -o $(TARGET).nostrip.sys vfat.o $(LIBS)
	- $(RM) temp.exp


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
