# $Id: Makefile,v 1.9 2001/07/04 21:24:02 chorns Exp $
#
#
PATH_TO_TOP = ../../../..

CFLAGS = -D__NTDRIVER__ -Iinclude

TARGETNAME = ne2000

RESOURCE_OBJECT  = $(TARGETNAME).coff
NE2000_OBJECTS   = ne2000/main.o ne2000/8390.o $(RESOURCE_OBJECT)
IMPORT_LIBS      = $(PATH_TO_TOP)/services/net/ndis/ndis.a \
                   $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

OBJECTS = $(NE2000_OBJECTS) $(IMPORT_LIBS)

all: $(TARGETNAME).sys

$(TARGETNAME).coff: $(TARGETNAME).rc $(PATH_TO_TOP)/include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = \
	    ne2000\*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = \
		ne2000/*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
endif

$(TARGETNAME).sys: $(OBJECTS)
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
        $(OBJECTS) \
        -mdll \
        -o junk.tmp \
        -Wl,--image-base,0x10000 \
        -Wl,--defsym,_end=end \
	    -Wl,--defsym,_edata=__data_end__ \
        -Wl,--defsym,_etext=etext \
	    -Wl,--base-file,base.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
        --dllname $(TARGETNAME).sys \
        --base-file base.tmp \
		--output-exp temp.exp \
        --def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
        $(OBJECTS) \
        -mdll \
        -o $(TARGETNAME).sys \
        -Wl,--image-base,0x10000 \
        -Wl,-e,_DriverEntry@8 \
        -Wl,--defsym,_end=end \
	    -Wl,--defsym,_edata=__data_end__ \
        -Wl,--defsym,_etext=etext \
        -Wl,temp.exp 
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).sys > $(TARGETNAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)
.phony: clean

install: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGETNAME).sys

$(TARGETNAME).sys: $(OBJECTS)

WITH_DEBUGGING=yes
include $(PATH_TO_TOP)/rules.mak
