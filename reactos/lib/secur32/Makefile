# $Id: Makefile,v 1.2 2000/08/18 22:27:02 dwelch Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET = secur32

SECUR32_BASE = 0x10000000

BASE_CFLAGS = -I../../include

CFLAGS = $(CFLAGS)

include ../../rules.mak
DLLTARGET=$(TARGET).dll

all: $(DLLTARGET)

OBJECTS = lsa.o dllmain.o

ifeq ($(DOSCLI),yes)
CLEAN_FILES = $(OBJECTS) \
              $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
	      $(TARGET).dll $(TARGET).sym $(TARGET).coff
else
CLEAN_FILES = $(OBJECTS) \
	      $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
	      $(TARGET).dll $(TARGET).sym $(TARGET).coff
endif

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

WARNINGS_ARE_ERRORS = yes

