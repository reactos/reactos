# $Id: makefile,v 1.13 2001/07/24 16:29:25 ekohl Exp $
#
# Linux Ext2 IFS Driver
#
PATH_TO_TOP = ../../..

TARGETNAME=ext2fs

OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o quota.o security.o \
          attr.o ext2.coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -g -Wall 

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) *.o ext2.coff junk.tmp base.tmp temp.exp $(TARGETNAME).sys

.phony: clean

ext2fs.sys: $(OBJECTS) $(LIBS)
	$(CC) \
		-nostartfiles \
		-nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,-e,_DriverEntry@8 \
		-Wl,--base-file,base.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-o junk.tmp \
		$(OBJECTS) $(LIBS)
	$(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--base-file base.tmp \
		--output-exp temp.exp
	$(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry \
		-Wl,temp.exp \
		-o $(TARGETNAME).sys \
		$(OBJECTS) $(LIBS)
	$(RM) temp.exp

ext2.coff: $(PATH_TO_TOP)/include/reactos/buildno.h ext2.rc

include $(PATH_TO_TOP)/rules.mak

# EOF
