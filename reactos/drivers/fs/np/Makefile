# $Id: Makefile,v 1.10 2001/07/28 14:46:27 ea Exp $
#
# Named Pipes File System
#
PATH_TO_TOP=../../..

TARGETNAME=npfs

CFLAGS=-D__NTDRIVER__ -I../../../include

OBJECTS=create.o finfo.o fsctrl.o mount.o rw.o volume.o npfs.coff

LIBS=$(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) *.o npfs.coff junk.tmp base.tmp temp.exp $(TARGETNAME).sys

.phony: clean

$(TARGETNAME).sys: $(OBJECTS) $(LIBS)
	$(CC) \
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
		--output-exp temp.exp \
		--kill-at
	$(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-mdll \
		-o $(TARGETNAME).sys \
		$(OBJECTS) $(LIBS) \
		-Wl,temp.exp
	$(RM) temp.exp
	$(NM) --numeric-sort npfs.sys > npfs.sym

$(TARGETNAME).coff: $(PATH_TO_TOP)/include/reactos/buildno.h npfs.rc

include $(PATH_TO_TOP)/rules.mak
