# $Id: makefile,v 1.14 2000/08/18 22:26:59 dwelch Exp $
#
# Makefile for ReactOS advapi32.dll
#
PATH_TO_TOP = ../..

BASE_CFLAGS = -I../../include

TARGETNAME=advapi32

DLLTARGET=$(TARGETNAME).dll



MISC_OBJECTS = misc/dllmain.o misc/shutdown.o \
		misc/sysfunc.o 

REGISTRY_OBJECTS = reg/reg.o

SECURITY_OBJECTS = sec/lsa.o sec/sec.o sec/ac.o sec/sid.o

SERVICE_OBJECTS = service/scm.o service/sctrl.o service/undoc.o

TOKEN_OBJECTS = token/token.o

RESOURCE_OBJECT = $(TARGETNAME).coff

OBJECTS = $(MISC_OBJECTS) $(REGISTRY_OBJECTS) $(SECURITY_OBJECTS) \
          $(SERVICE_OBJECTS) $(TOKEN_OBJECTS) \
	  $(RESOURCE_OBJECT)

ifeq ($(DOSCLI),yes)
CLEAN_FILES = misc\*.o reg\*.o sec\*.o service\*.o token\*.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	      $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
else
CLEAN_FILES = misc/*.o reg/*.o sec/*.o service/*.o token/*.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	      $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
endif

all: $(DLLTARGET)

$(TARGETNAME).coff: $(TARGETNAME).rc ../../include/reactos/resource.h

$(TARGETNAME).a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).a

$(TARGETNAME).dll: $(DLLMAIN) $(OBJECTS) $(TARGETNAME).def
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
		-specs=$(TARGETNAME)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
		$(TARGETNAME).o \
		../ntdll/ntdll.a \
		../kernel32/kernel32.a \
		-specs=$(TARGETNAME)_specs \
		-mdll \
		-o $(TARGETNAME).dll \
		-Wl,--image-base,0x20000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).dll > $(TARGETNAME).sym


clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll

$(FLOPPY_DIR)/dlls/$(TARGETNAME).dll: $(TARGETNAME).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).dll $(FLOPPY_DIR)\dlls\$(TARGETNAME).dll
else
	$(CP) $(TARGETNAME).dll $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll
endif

dist: $(DIST_DIR)/dlls/$(TARGETNAME).dll

$(DIST_DIR)/dlls/$(TARGETNAME).dll: $(TARGETNAME).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).dll ..\..\$(DIST_DIR)\dlls\$(TARGETNAME).dll
else
	$(CP) $(TARGETNAME).dll ../../$(DIST_DIR)/dlls/$(TARGETNAME).dll
endif

include ../../rules.mak

