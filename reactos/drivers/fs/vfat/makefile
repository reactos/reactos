#
#
#
OBJECTS = blockdev.o dir.o dirwr.o iface.o ../../../ntoskrnl/ntoskrnl.a

all: vfatfsd.sys

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
	- $(RM) vfatfsd.sys
else
	- $(RM) blockdev.o dir.o dirwr.o iface.o junk.tmp base.tmp temp.exp vfatfsd.sys
endif

.phony: clean

floppy: $(FLOPPY_DIR)/drivers/vfatfsd.sys

$(FLOPPY_DIR)/drivers/vfatfsd.sys: vfatfsd.sys
ifeq ($(DOSCLI),yes)
	$(CP) vfatfsd.sys $(FLOPPY_DIR)\vfatfsd.sys
else
	$(CP) vfatfsd.sys $(FLOPPY_DIR)/vfatfsd.sys
endif

vfatfsd.sys: $(OBJECTS)
	$(CC) -specs=../../svc_specs -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname vfatfsd.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry@8 \
	      -specs=../../svc_specs -mdll -o vfatfsd.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp


