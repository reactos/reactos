# $Id: makefile,v 1.3 1999/10/21 16:11:42 ekohl Exp $
#
# ReactOS psxdll.dll makefile
#
TARGETNAME=psxdll

ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=$(TARGETNAME).a
  else
    DLLTARGET=$(TARGETNAME).dll
  endif
else
  DLLTARGET=$(TARGETNAME).dll
endif

BASE_CFLAGS = -I../../include

all: $(DLLTARGET)


OBJECTS_LIBC_STDLIB = libc/stdlib/exit.o

OBJECTS_LIBC = $(OBJECTS_LIBC_STDLIB)

OBJECTS_MISC = misc/dllmain.o misc/special.o misc/heap.o misc/rtl.o

OBJECTS_RESOURCE = $(TARGETNAME).coff

OBJECTS = $(OBJECTS_LIBC) $(OBJECTS_LIBM) $(OBJECTS_MISC) $(OBJECTS_RESOURCE)

DEFS=$(TARGETNAME).def $(TARGETNAME).edf

$(TARGETNAME).coff: $(TARGETNAME).rc ../../include/reactos/resource.h

$(TARGETNAME).a: $(OBJECTS)
	$(LD) \
		-r $(OBJECTS) \
		-o $(TARGETNAME).a

$(TARGETNAME).dll: $(DLLMAIN) $(OBJECTS) $(DEFS)
	$(LD) \
		-r $(OBJECTS) \
		-o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME) \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		-specs=$(TARGETNAME)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGETNAME).o \
		../ntdll/ntdll.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME) \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
		-specs=$(TARGETNAME)_specs \
		-mdll \
		-o $(TARGETNAME).dll \
		$(TARGETNAME).o \
		../ntdll/ntdll.a\
		-Wl,--entry,_DllMain@12 \
		-Wl,--image-base,0x75390000 \
		-Wl,--file-alignment,0x200 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--subsystem,posix:19.90 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).dll > $(TARGETNAME).sym


ifeq ($(DOSCLI),yes)
CLEAN_FILES=*.o *.a misc\*.o
else
CLEAN_FILES=*.o *.a misc/*.o
endif

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

floppy: $(FLOPPY_DIR)/dlls/$(TARGETNAME).dll

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

#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

# EOF
