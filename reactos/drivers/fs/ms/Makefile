# $Id: Makefile,v 1.3 2001/06/22 12:26:46 ekohl Exp $
#
# Mailslot File System
#
PATH_TO_TOP = ../../..

TARGETNAME=msfs

OBJECTS = create.o finfo.o fsctrl.o msfs.o rw.o msfs.coff

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

CFLAGS = -D__NTDRIVER__

all: $(TARGETNAME).sys

.phony: all

clean:
	- $(RM) *.o 
	- $(RM) $(TARGETNAME).sym
	- $(RM) $(TARGETNAME).sys

.phony: clean

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys

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

msfs.coff: $(PATH_TO_TOP)/include/reactos/buildno.h msfs.rc

include $(PATH_TO_TOP)/rules.mak
