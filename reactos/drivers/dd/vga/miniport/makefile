# $Id: makefile,v 1.7 2001/07/15 21:18:59 rex Exp $
#
#
PATH_TO_TOP = ../../../..
CFLAGS = -D__NTDRIVER__

MP_OBJECTS = vgamp.o initvga.o vgavideo.o vgamp.coff ../../../../ntoskrnl/ntoskrnl.a ../../vidport/vidport.a

all: vgamp.sys

.phony: all

clean:
	- $(RM) *.o junk.tmp base.tmp temp.exp *.sys *.coff

.phony: clean

vgamp.sys: $(MP_OBJECTS)
	$(CC) \
		-specs=../../../svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
		$(MP_OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname vgamp.sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-specs=../../../svc_specs \
		-mdll \
		-o vgamp.sys \
		$(MP_OBJECTS)
	- $(RM) temp.exp

install: $(FLOPPY_DIR)/drivers/vgamp.sys

$(FLOPPY_DIR)/drivers/vgamp.sys: vgamp.sys
	$(CP) vgamp.sys $(FLOPPY_DIR)/drivers/vgamp.sys

dist: ../../../../$(DIST_DIR)/drivers/vgamp.sys

../../../../$(DIST_DIR)/drivers/vgamp.sys: vgamp.sys
	$(CP) vgamp.sys ../../../../$(DIST_DIR)/drivers/vgamp.sys

include ../../../../rules.mak

