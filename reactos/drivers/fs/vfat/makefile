# $Id: makefile,v 1.12 1999/12/11 21:14:49 dwelch Exp $
#
#
TARGET=vfatfs

OBJECTS = blockdev.o dir.o dirwr.o iface.o string.o fat.o rw.o finfo.o \
          volume.o create.o
LIBS = ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I../../../include

all: $(TARGET).sys

.phony: all

clean:
ifeq ($(DOSCLI),yes)
	- $(RM) blockdev.o
	- $(RM) dir.o
	- $(RM) dirwr.o
	- $(RM) iface.o
	- $(RM) $(TARGET).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGET).sys
else
	- $(RM) $(OBJECTS) $(TARGET).coff junk.tmp base.tmp temp.exp $(TARGET).sys
endif

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys $(FLOPPY_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys
endif

dist: ../../../$(DIST_DIR)/drivers/$(TARGET).sys

../../../$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys ..\..\..\$(DIST_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys ../../../$(DIST_DIR)/drivers/$(TARGET).sys
endif

$(TARGET).sys: $(OBJECTS) $(LIBS)
	$(CC) \
		-specs=../../svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
		-Wl,"-h vfatfs.sys" \
		$(OBJECTS) $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-Wl,"-h vfatfs.sys" \
		-specs=../../svc_specs \
		-mdll \
		-o $(TARGET).sys \
		$(OBJECTS) $(LIBS)
	- $(RM) temp.exp


WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
