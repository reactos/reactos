# $Id: makefile,v 1.12 2001/07/15 21:18:59 rex Exp $
#
# Linux Ext2 IFS Driver
#
PATH_TO_TOP = ../../..

TARGETNAME=ext2fs

OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o quota.o security.o \
          attr.o ext2.coff

LIBS = ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -g -Wall 

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) *.o ext2.coff junk.tmp base.tmp temp.exp $(TARGETNAME).sys

.phony: clean

ext2fs.sys: $(OBJECTS) $(LIBS)
	$(CC) \
		-specs=../../svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
		$(OBJECTS) $(LIBS)
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
		$(OBJECTS) $(LIBS) \
		-Wl,temp.exp
	$(RM) temp.exp

ext2.coff: ../../../include/reactos/buildno.h ext2.rc

include ../../../rules.mak

# EOF
