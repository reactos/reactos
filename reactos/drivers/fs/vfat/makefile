#
#
#
TARGET=vfatfs
OBJECTS = blockdev.o dir.o dirwr.o iface.o ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I../../../include

all: vfatfs.sys

.phony: all

clean:
ifeq ($(DOSCLI),yes)
	- $(RM) blockdev.o
	- $(RM) dir.o
	- $(RM) dirwr.o
	- $(RM) iface.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) vfatfs.sys
else
	- $(RM) blockdev.o dir.o dirwr.o iface.o junk.tmp base.tmp temp.exp vfatfs.sys
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

vfatfs.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp -Wl,"-h vfatfs.sys" $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname vfatfs.sys --base-file base.tmp \
	           --output-exp temp.exp --def vfatfs.def
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o vfatfs.sys $(OBJECTS) \
	      -Wl,temp.exp -Wl,"-h vfatfs.sys"
	- $(RM) temp.exp


include ../../../rules.mak
