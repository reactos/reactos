all: test.sys

OBJECTS = test.o ../../../ntoskrnl/ntoskrnl.a

test.sys: test.o
	$(DLLTOOL) --dllname test.sys --def test.def \
	           --output-lib test.a
	$(CC) -specs=$(KM_SPECS) -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	$(RM) junk.tmp
	$(DLLTOOL) --dllname test.sys --base-file base.tmp \
	           --output-exp temp.exp --def test.def
	$(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry -specs=$(KM_SPECS) -mdll -o test.sys $(OBJECTS) -Wl,temp.exp
	$(RM) temp.exp
