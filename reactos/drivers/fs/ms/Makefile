# $Id: Makefile,v 1.1 2001/05/05 15:11:57 ekohl Exp $
#
# Mailslot File System
#
PATH_TO_TOP = ../../..

TARGETNAME=msfs

OBJECTS = create.o finfo.o fsctrl.o msfs.o rw.o msfs.coff

LIBS = ../../../ntoskrnl/ntoskrnl.a

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) *.o 
	- $(RM) $(TARGETNAME).sym
	- $(RM) $(TARGETNAME).sys

.phony: clean

$(TARGETNAME).sys: $(OBJECTS) $(LIBS)
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
		--output-exp temp.exp \
		--kill-at
	$(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-specs=../../svc_specs \
		-mdll \
		-o $(TARGETNAME).sys \
		$(OBJECTS) $(LIBS) \
		-Wl,temp.exp
	$(RM) temp.exp
	$(NM) --numeric-sort msfs.sys > msfs.sym

msfs.coff: ../../../include/reactos/buildno.h msfs.rc

include ../../../rules.mak
