all: vfatfsd.sys

OBJECTS = blockdev.o dir.o dirwr.o iface.o ../../../ntoskrnl/ntoskrnl.a

vfatfsd.sys: $(OBJECTS)
	$(CC) -specs=../../../$(KM_SPECS) -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname vfatfsd.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry \
	      -specs=$(KM_SPECS) -mdll -o vfatfsd.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp
