all: ext2fs.sys

OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o \
          ../../../ntoskrnl/ntoskrnl.a

ext2fs.sys: $(OBJECTS)
	$(CC) -specs=$(KM_SPECS) -mdll -o junk.tmp -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ -Wl,--defsym,_etext=etext \
	      -Wl,--base-file,base.tmp $(OBJECTS)
	$(RM) junk.tmp
	$(DLLTOOL) --dllname ext2fs.sys --base-file base.tmp \
	           --output-exp temp.exp
	$(RM) base.tmp
	$(CC) --verbose -Wl,--image-base,0x10000 -Wl,-e,_DriverEntry \
	      -specs=$(KM_SPECS) -mdll -o ext2fs.sys $(OBJECTS) -Wl,temp.exp
	$(RM) temp.exp
