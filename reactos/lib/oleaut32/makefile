# $Id: makefile,v 1.2 2001/07/21 15:34:08 ea Exp $
#
# Makefile for ReactOS oleaut32.dll
#
PATH_TO_TOP = ../..

BASE_CFLAGS = -I../../include

TARGETNAME=oleaut32

IMAGE_BASE = 0x76260000

DLLTARGET=$(TARGETNAME).dll
OBJECTS = dllmain.o stubs.o $(TARGETNAME).coff
CLEAN_FILES = $(OBJECTS) $(TARGETNAME).o $(TARGETNAME).a junk.tmp  \
	base.tmp temp.exp $(TARGETNAME).dll $(TARGETNAME).sym  \
	$(TARGETNAME).coff

all: $(DLLTARGET)

$(TARGETNAME).coff: $(TARGETNAME).rc $(PATH_TO_TOP)/include/reactos/resource.h

$(TARGETNAME).a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).a

$(TARGETNAME).dll: $(OBJECTS) $(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		$(TARGETNAME).o \
		../ntdll/ntdll.a \
		../kernel32/kernel32.a \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--entry=_DllMain@12\
		-Wl,--base-file,base.tmp 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).def
	- $(RM) base.tmp
	$(CC) \
		$(TARGETNAME).o \
		../ntdll/ntdll.a \
		../kernel32/kernel32.a \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o $(TARGETNAME).dll \
		-Wl,--entry=_DllMain@12\
		-Wl,--image-base,$(IMAGE_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).dll > $(TARGETNAME).sym


clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll

$(FLOPPY_DIR)/dlls/$(TARGETNAME).dll: $(TARGETNAME).dll
	$(CP) $(TARGETNAME).dll $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll


dist: $(DIST_DIR)/dlls/$(TARGETNAME).dll

$(DIST_DIR)/dlls/$(TARGETNAME).dll: $(TARGETNAME).dll
	$(CP) $(TARGETNAME).dll ../../$(DIST_DIR)/dlls/$(TARGETNAME).dll


include ../../rules.mak

