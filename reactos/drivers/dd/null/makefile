# $Id: makefile,v 1.12 2001/07/15 21:18:58 rex Exp $
#
#
PATH_TO_TOP = ../../..

TARGETNAME=null
OBJECTS= $(TARGETNAME).o $(TARGETNAME).coff ../../../ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__ -I../../../include

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) $(TARGETNAME).o $(TARGETNAME).coff junk.tmp base.tmp temp.exp $(TARGETNAME).sys

.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys

../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys

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
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--verbose \
		-Wl,--image-base,0x10000 \
		-Wl,-e,_DriverEntry@8 \
		-Wl,temp.exp \
		-specs=../../svc_specs \
		-mdll \
		-o $(TARGETNAME).sys \
		$(OBJECTS)
	- $(RM) temp.exp


include ../../../rules.mak
