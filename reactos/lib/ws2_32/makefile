# Makefile for ReactOS WinSock 2 DLL

PATH_TO_TOP = ../..

BASE_CFLAGS = -Iinclude -I../../include -DUNICODE

TARGETNAME=ws2_32

MISC_OBJECTS = misc/dllmain.o misc/catalog.o misc/event.o misc/ns.o \
               misc/stubs.o misc/upcall.o

RESOURCE_OBJECT = $(TARGETNAME).coff

OBJECTS = $(MISC_OBJECTS) $(RESOURCE_OBJECT)

LIBS = ../ntdll/ntdll.a \
       ../kernel32/kernel32.a \
       ../advapi32/advapi32.a

ifeq ($(DOSCLI),yes)
CLEAN_FILES = misc\*.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	          $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
else
CLEAN_FILES = misc/*.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	          $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
endif

all: $(TARGETNAME).dll

$(TARGETNAME).coff: $(TARGETNAME).rc ../../include/reactos/resource.h

$(TARGETNAME).a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).a

$(TARGETNAME).dll: $(LIBS) $(OBJECTS) $(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		$(TARGETNAME).o \
        $(LIBS) \
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
        $(LIBS) \
		-specs=$(TARGETNAME)_specs \
		-mdll \
		-o $(TARGETNAME).dll \
		-Wl,--image-base,0x10000 \
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

