# $Id: Makefile,v 1.4 2000/10/07 13:41:49 dwelch Exp $
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
	      $(TARGET).dll $(TARGET).sym $(TARGET).coff

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

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

include $(PATH_TO_TOP)/rules.mak

