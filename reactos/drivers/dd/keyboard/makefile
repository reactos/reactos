all: keyboard.sys

OBJECTS = keyboard.o ../../../ntoskrnl/ntoskrnl.a

keyboard.sys: $(OBJECTS)
	$(CC) -specs=../../../$(KM_SPECS) -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname keyboard.sys --base-file base.tmp \
	           --output-exp temp.exp
	- $(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry -specs=$(KM_SPECS) -mdll -o keyboard.sys $(OBJECTS) -Wl,temp.exp
	- $(RM) temp.exp
