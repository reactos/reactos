# Makefile for ReactOS WinSock Helper DLL for TCP/IP

PATH_TO_TOP = ../../..

CFLAGS = -D__NTDRIVER__ -I./ -DUNICODE

TARGETNAME=wshtcpip

MAIN_OBJECTS = $(TARGETNAME).o

RESOURCE_OBJECT = $(TARGETNAME).coff

OBJECTS = $(MAIN_OBJECTS) $(RESOURCE_OBJECT)

LIBS = ../../../lib/ntdll/ntdll.a \
       ../../../lib/kernel32/kernel32.a \
       ../../../lib/ws2_32/ws2_32.a

ifeq ($(DOSCLI),yes)
CLEAN_FILES = $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	          $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
else
CLEAN_FILES = $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	          $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
endif

all: $(TARGETNAME).dll

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

$(TARGETNAME).dll: $(LIBS) $(OBJECTS) $(TARGETNAME).def
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		$(TARGETNAME).o \
        $(LIBS) \
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
		$(OBJECTS) $(LIBS) \
		-mdll \
		-o $(TARGETNAME).dll \
		-Wl,--image-base,0x777C0000 \
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
	$(CP) $(TARGETNAME).dll $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll

dist: $(DIST_DIR)/dlls/$(TARGETNAME).dll

$(DIST_DIR)/dlls/$(TARGETNAME).dll: $(TARGETNAME).dll
	$(CP) $(TARGETNAME).dll ../../../$(DIST_DIR)/dlls/$(TARGETNAME).dll

include ../../../rules.mak

