all: ide.sys

OBJECTS = ide.o ../../../ntoskrnl/ntoskrnl.a

ide.sys: $(OBJECTS)
	$(DLLTOOL) --dllname ide.sys --def ide.def \
	           --output-lib ide.a
	$(CC) -specs=$(KM_SPECS) -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	$(RM) junk.tmp
	$(DLLTOOL) --dllname ide.sys --base-file base.tmp \
	           --output-exp temp.exp --def ide.def
	$(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry -specs=$(KM_SPECS) -mdll -o ide.sys $(OBJECTS) -Wl,temp.exp
	$(RM) temp.exp
