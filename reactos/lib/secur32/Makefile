# $Id: Makefile,v 1.6 2001/07/15 21:18:55 rex Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

CFLAGS = -Iinclude -D__SECUR32__ -DDBG -g -Wall -Werror

TARGET = secur32

SECUR32_BASE = 0x10000000

DLLTARGET=$(TARGET).dll

all: $(DLLTARGET)

OBJECTS = lsa.o dllmain.o $(TARGET).coff

CLEAN_FILES = $(OBJECTS) \
	      $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
	      $(TARGET).dll $(TARGET).sym

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) \
		-r $(OBJECTS) \
		-o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGET).o \
		../ntdll/ntdll.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
		-mdll \
		-o $(TARGET).dll \
		$(TARGET).o \
		../ntdll/ntdll.a \
		-Wl,--image-base,$(SECUR32_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll ../../$(DIST_DIR)/dlls/$(TARGET).dll

include $(PATH_TO_TOP)/rules.mak

