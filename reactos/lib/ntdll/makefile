# $Id: makefile,v 1.56 2000/12/23 02:37:37 dwelch Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET = ntdll

CFLAGS = -D__NTDLL__ -g

IMAGE_BASE = 0x77f60000

DLLTARGET=$(TARGET).dll
DLLMAIN=main/dllmain.o

all: $(DLLTARGET)

CSR_OBJECTS = csr/api.o

DBG_OBJECTS = dbg/brkpoint.o dbg/debug.o dbg/print.o

RTL_OBJECTS = rtl/critical.o rtl/error.o rtl/heap.o rtl/largeint.o \
              rtl/math.o rtl/mem.o rtl/nls.o rtl/process.o rtl/sd.o \
              rtl/thread.o rtl/unicode.o rtl/env.o rtl/path.o rtl/ppb.o \
              rtl/bitmap.o rtl/time.o rtl/acl.o rtl/sid.o rtl/image.o \
              rtl/access.o rtl/apc.o rtl/callback.o rtl/luid.o rtl/misc.o \
              rtl/registry.o rtl/exception.o

STDIO_OBJECTS = stdio/sprintf.o stdio/swprintf.o

STDLIB_OBJECTS = stdlib/abs.o stdlib/atoi.o stdlib/atoi64.o stdlib/atol.o \
                 stdlib/itoa.o stdlib/itow.o stdlib/labs.o stdlib/splitp.o \
                 stdlib/strtol.o stdlib/strtoul.o stdlib/wcstol.o \
                 stdlib/wcstoul.o stdlib/wtoi.o stdlib/wtoi64.o stdlib/wtol.o \
                 stdlib/mbstowcs.o stdlib/wcstombs.o stdlib/qsort.o

STRING_OBJECTS = string/ctype.o string/memccpy.o string/memchr.o \
                 string/memcmp.o string/memcpy.o string/memicmp.o\
                 string/memmove.o string/memset.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcspn.o \
                 string/strcpy.o string/stricmp.o string/strlen.o \
                 string/strlwr.o string/strncat.o string/strncmp.o \
                 string/strncpy.o string/strnicmp.o string/strnlen.o \
                 string/strpbrk.o string/strrchr.o string/strspn.o \
                 string/strstr.o string/strupr.o string/wstring.o

OBJECTS = napi.o ldr/startup.o $(DBG_OBJECTS) $(RTL_OBJECTS) \
          $(STDIO_OBJECTS) $(STDLIB_OBJECTS) $(STRING_OBJECTS) \
          stubs/stubs.o ldr/utils.o $(CSR_OBJECTS) $(TARGET).coff

ifeq ($(DOSCLI),yes)
CLEAN_FILES = napi.o ldr\*.o rtl\*.o stdio\*.o stdlib\*.o string\*.o stubs\*.o \
              $(TARGET).a main\dllmain.o $(TARGET).o $(TARGET).dll junk.tmp \
              base.tmp temp.exp $(TARGET).sym $(TARGET).coff
else
CLEAN_FILES = napi.o ldr/*.o rtl/*.o stdio/*.o stdlib/*.o string/*.o stubs/*.o csr/*.o dbg/*.o \
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
		-nostartfiles -nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGET).o -lgcc
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def def/$(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		-mdll \
		-o $(TARGET).dll \
		$(TARGET).o -lgcc \
		-Wl,--entry=_LdrInitializeThunk@16 \
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

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll ../../$(DIST_DIR)/dlls/$(TARGET).dll

include ../../rules.mak
