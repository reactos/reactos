# $Id: makefile,v 1.7 1999/08/29 13:44:52 dwelch Exp $
#
# Makefile for ReactOS advapi32.dll
#

BASE_CFLAGS = -I../../include

TARGET=advapi32
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=$(TARGET).a
  else
    DLLTARGET=$(TARGET).dll
  endif
else
  DLLTARGET=$(TARGET).dll
endif


MISC_OBJECTS = misc/dllmain.o misc/shutdown.o \
		misc/sysfunc.o misc/stubs.o

REGISTRY_OBJECTS = reg/reg.o

SECURITY_OBJECTS = sec/lsa.o sec/sec.o
#SECURITY_OBJECTS = sec/lsa.o sec/rtlsec.o sec/sec.o

SERVICE_OBJECTS = service/scm.o

TOKEN_OBJECTS = token/token.o

RESOURCE_OBJECT = advapi32.coff

OBJECTS = $(MISC_OBJECTS) $(REGISTRY_OBJECTS) $(SECURITY_OBJECTS) \
          $(SERVICE_OBJECTS) $(TOKEN_OBJECTS) \
	  $(RESOURCE_OBJECT)

all: $(DLLTARGET)

$(TARGET).a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET).a

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) \
		$(TARGET).o \
		../ntdll/ntdll.a \
		../kernel32/kernel32.a \
		-specs=$(TARGET)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
		$(TARGET).o \
		../ntdll/ntdll.a \
		../kernel32/kernel32.a \
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		-Wl,--image-base,0x20000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

clean:

include ../../rules.mak

