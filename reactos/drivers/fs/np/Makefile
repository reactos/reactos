# $Id: Makefile,v 1.3 2000/04/01 14:20:21 ea Exp $
#
# Named Pipes File System
#

TARGETNAME=npfs

BASE_CFLAGS = -I../../../include

OBJECTS = create.o mount.o npfs.coff \
          ../../../ntoskrnl/ntoskrnl.a

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) create.o 
	- $(RM) mount.o
	- $(RM) $(TARGETNAME).sys

.phony: clean

$(TARGETNAME).sys: $(OBJECTS)
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
		--output-exp temp.exp \
		--kill-at
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

npfs.coff: ../../../include/reactos/buildno.h npfs.rc

include ../../../rules.mak
