# $Id: makefile,v 1.34 2001/07/24 16:32:16 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGET = vfatfs

OBJECTS = blockdev.o cleanup.o close.o create.o dir.o direntry.o dirwr.o  \
          fat.o fcb.o finfo.o iface.o rw.o shutdown.o string.o volume.o  \
          $(TARGET).coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -g -Wall -Werror

all: $(TARGET).sys $(TARGET).sys.unstripped

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

#$(TARGET).sys $(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)

$(TARGET).sys.unstripped: $(OBJECTS) $(LIBS)
	$(LD) -r -o vfat.o $(OBJECTS)
	$(CC) \
		-nostartfiles \
		-nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,-e,_DriverEntry@8 \
		-Wl,--base-file,base.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		vfat.o $(LIBS) \
		-o junk.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
	 	-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-o $(TARGET).sys.unstripped \
		vfat.o $(LIBS)
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).sys.unstripped >$(TARGET).sym


$(TARGET).sys: $(OBJECTS) $(LIBS)
	$(LD) -r -o vfat.o $(OBJECTS)
	$(STRIP) --strip-debug vfat.o
	$(CC) \
		-nostartfiles \
		-nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,-e,_DriverEntry@8 \
		-Wl,--base-file,base.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		vfat.o $(LIBS) \
		-o junk.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
	 	-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-o $(TARGET).sys \
		vfat.o $(LIBS)
	- $(RM) temp.exp

$(TARGET).coff: $(PATH_TO_TOP)/include/reactos/buildno.h vfatfs.rc

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes
include $(PATH_TO_TOP)/rules.mak
