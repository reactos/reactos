# $Id: makefile,v 1.5 1999/12/18 10:17:42 ea Exp $
#
#
BASE_CFLAGS = -I../../../include

MP_OBJECTS = vgamp.o vgamp.coff ../../../ntoskrnl/ntoskrnl.a ../vidport/vidport.a
DDI_OBJECTS = vgaddi.o vgaddi.coff ../../../subsys/win32k/win32k.a

all: vgamp.sys vgaddi.sys

.phony: all

clean:
	- $(RM) *.o
	- $(RM) junk.tmp
	- $(RM) base.tmp
	- $(RM) temp.exp
	- $(RM) *.sys
	- $(RM) *.coff

.phony: clean

vgamp.sys: $(MP_OBJECTS)
	$(CC) \
		-specs=../../svc_specs \
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
		-specs=../../svc_specs \
		-mdll \
		-o vgamp.sys \
		$(MP_OBJECTS)
	- $(RM) temp.exp

vgaddi.sys: $(DDI_OBJECTS)
	$(CC) \
		-specs=../../svc_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp \
		$(DDI_OBJECTS)
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname vgaddi.sys \
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
		-o vgaddi.sys \
		$(DDI_OBJECTS)
	- $(RM) temp.exp

floppy: $(FLOPPY_DIR)/drivers/vgamp.sys $(FLOPPY_DIR)/drivers/vgaddi.sys

$(FLOPPY_DIR)/drivers/vgamp.sys: vgamp.sys
ifeq ($(DOSCLI),yes)
	$(CP) vgamp.sys $(FLOPPY_DIR)\drivers\vgamp.sys
else
	$(CP) vgamp.sys $(FLOPPY_DIR)/drivers/vgamp.sys
endif

$(FLOPPY_DIR)/drivers/vgaddi.sys: vgaddi.sys
ifeq ($(DOSCLI),yes)
	$(CP) vgaddi.sys $(FLOPPY_DIR)\drivers\vgaddi.sys
else
	$(CP) vgaddi.sys $(FLOPPY_DIR)/drivers/vgaddi.sys
endif

dist: ../../../$(DIST_DIR)/drivers/vgamp.sys ../../../$(DIST_DIR)/drivers/vgaddi.sys

../../../$(DIST_DIR)/drivers/vgamp.sys: vgamp.sys
ifeq ($(DOSCLI),yes)
	$(CP) vgamp.sys ..\..\..\$(DIST_DIR)\drivers\vgamp.sys
else
	$(CP) vgamp.sys ../../../$(DIST_DIR)/drivers/vgamp.sys
endif

../../../$(DIST_DIR)/drivers/vgaddi.sys: vgaddi.sys
ifeq ($(DOSCLI),yes)
	$(CP) vgaddi.sys ..\..\..\$(DIST_DIR)\drivers\vgaddi.sys
else
	$(CP) vgaddi.sys ../../../$(DIST_DIR)/drivers/vgaddi.sys
endif

include ../../../rules.mak

