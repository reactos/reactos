# $Id: makefile,v 1.8 2000/04/01 14:19:07 ea Exp $
#
# Linux Ext2 IFS Driver
#
BASE_CFLAGS = -I../../../include

TARGETNAME=ext2fs

OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o quota.o security.o \
          attr.o ext2.coff ../../../ntoskrnl/ntoskrnl.a

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) super.o 
	- $(RM) blockdev.o
	- $(RM) inode.o
	- $(RM) file.o
	- $(RM) dir.o
	- $(RM) rw.o
	- $(RM) ext2.coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGETNAME).sys

.phony: clean

ext2fs.sys: $(OBJECTS)
	$(CC) \
		-specs=../../svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
		$(OBJECTS)
	$(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--base-file base.tmp \
		--output-exp temp.exp
	$(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry \
		-specs=../../svc_specs \
		-mdll \
		-o $(TARGETNAME).sys \
		$(OBJECTS) \
		-Wl,temp.exp
	$(RM) temp.exp

ext2.coff: ../../../include/reactos/buildno.h ext2.rc

include ../../../rules.mak

# EOF
