# $Id: makefile,v 1.20 1999/07/22 21:31:39 ekohl Exp $
#
# ReactOS Operating System
#
TARGET=ntdll

IMAGE_BASE = 0x77f60000

ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=$(TARGET).a
    DLLMAIN=
  else
    DLLTARGET=$(TARGET).dll
    DLLMAIN=main/dllmain.o
  endif
else
  DLLTARGET=$(TARGET).dll
  DLLMAIN=main/dllmain.o
endif

all: $(DLLTARGET)

RTL_OBJECTS = rtl/critical.o rtl/error.o rtl/heap.o rtl/largeint.o \
              rtl/mem.o rtl/namespc.o rtl/security.o rtl/unicode.o

STDLIB_OBJECTS = stdlib/atoi.o stdlib/atol.o stdlib/splitp.o \
                 stdlib/strtol.o stdlib/strtoul.o

STRING_OBJECTS = string/ctype.o string/memcpy.o string/memmove.o \
                 string/memset.o string/strcat.o string/strchr.o \
                 string/strcmp.o string/strcspn.o \
                 string/strcpy.o string/stricmp.o string/strlen.o string/strlwr.o \
                 string/strncat.o string/strncmp.o string/strncpy.o \
                 string/strnicmp.o string/strnlen.o string/strrchr.o \
                 string/strstr.o string/strupr.o string/wstring.o

OBJECTS = napi.o ldr/startup.o $(RTL_OBJECTS) \
          stdio/vsprintf.o $(STDLIB_OBJECTS) $(STRING_OBJECTS) \
          stubs/stubs.o ldr/utils.o $(TARGET).coff

ifeq ($(DOSCLI),yes)
CLEAN_FILES = napi.o ldr\*.o rtl\*.o stdio\*.o stdlib\*.o string\*.o stubs\*.o \
              $(TARGET).a main\dllmain.o $(TARGET).o $(TARGET).dll junk.tmp \
	      base.tmp temp.exp $(TARGET).sym $(TARGET).coff
else
CLEAN_FILES = napi.o ldr/*.o rtl/*.o stdio/*.o stdlib/*.o string/*.o stubs/*.o \
              $(TARGET).a main/dllmain.o $(TARGET).o $(TARGET).dll junk.tmp \
	      base.tmp temp.exp $(TARGET).sym $(TARGET).coff
endif

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) def/ntdll.def def/ntdll.edf
	$(LD) \
		-r $(DLLMAIN) \
		$(OBJECTS) \
		-o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def def/$(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) \
		-specs=$(TARGET)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGET).o
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def def/$(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		$(TARGET).o \
		-Wl,--entry=_LdrStartup \
		-Wl,--image-base,$(IMAGE_BASE) \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

floppy: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).dll $(FLOPPY_DIR)\dlls\$(TARGET).dll
else
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll
endif

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).dll ..\..\$(DIST_DIR)\dlls\$(TARGET).dll
else
	$(CP) $(TARGET).dll ../../$(DIST_DIR)/dlls/$(TARGET).dll
endif

#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

