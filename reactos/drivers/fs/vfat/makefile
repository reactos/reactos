# $Id: makefile,v 1.20 2000/08/11 12:44:40 ekohl Exp $
#
#
TARGET=vfatfs

OBJECTS = blockdev.o close.o create.o dir.o dirwr.o iface.o string.o fat.o \
          rw.o finfo.o volume.o $(TARGET).coff
LIBS = ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I. -I../../../include

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

$(TARGET).nostrip.sys: $(OBJECTS) $(LIBS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" $(OBJECTS) $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 -Wl,temp.exp -Wl,"-h vfatfs.sys" -specs=../../svc_specs -mdll -o $(TARGET).nostrip.sys $(OBJECTS) $(LIBS)
	- $(RM) temp.exp


$(TARGET).sys: $(OBJECTS) $(LIBS)
	$(STRIP) --strip-debug $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" $(OBJECTS) $(LIBS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def vfatfs.def
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 -Wl,temp.exp -Wl,"-h vfatfs.sys" -specs=../../svc_specs -mdll -o $(TARGET).sys $(OBJECTS) $(LIBS)
	- $(RM) temp.exp

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
