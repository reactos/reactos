#
#
#
OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o \
          ../../../ntoskrnl/ntoskrnl.a

all: ext2fs.sys

.phony: all

clean:
	- $(RM) super.o 
	- $(RM) blockdev.o
	- $(RM) inode.o
	- $(RM) file.o
	- $(RM) dir.o
	- $(RM) rw.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) ext2fs.sys

.phony: clean

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

