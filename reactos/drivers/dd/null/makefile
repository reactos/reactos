# $Id: makefile,v 1.10 2000/11/20 19:59:10 ekohl Exp $
#
#
PATH_TO_TOP = ../../..

TARGETNAME=null
OBJECTS= $(TARGETNAME).o $(TARGETNAME).coff ../../../ntoskrnl/ntoskrnl.a

BASE_CFLAGS = -I../../../include

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) $(TARGETNAME).o
	- $(RM) $(TARGETNAME).coff
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) $(TARGETNAME).sys

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
