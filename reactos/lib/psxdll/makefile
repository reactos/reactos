# $Id: makefile,v 1.8 2000/08/18 22:27:01 dwelch Exp $
#
# ReactOS psxdll.dll makefile
#
PATH_TO_TOP = ../..

TARGETNAME=psxdll

DLLTARGET=$(TARGETNAME).dll


BASE_CFLAGS =-Iinclude -I../../include -D__PSXDLL__

all: $(DLLTARGET)


OBJECTS_LIBC_IO = libc/io/dup.o libc/io/dup2.o libc/io/fcntl.o

OBJECTS_LIBC_STDLIB = libc/stdlib/exit.o

OBJECTS_LIBC_SYS = libc/sys/uname.o

OBJECTS_LIBC_TIME = libc/time/gettimeofday.o libc/time/time.o

OBJECTS_LIBC = \
	$(OBJECTS_LIBC_IO) \
	$(OBJECTS_LIBC_STDLIB) \
	$(OBJECTS_LIBC_SYS) \
	$(OBJECTS_LIBC_TIME)

OBJECTS_MISC = misc/dllmain.o misc/special.o misc/heap.o misc/rtl.o

OBJECTS_RESOURCE = $(TARGETNAME).coff

OBJECTS = \
	$(OBJECTS_LIBC) \
	$(OBJECTS_LIBM) \
	$(OBJECTS_MISC) \
	$(OBJECTS_RESOURCE)

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
CLEAN_FILES = *.o *.a misc\*.o libc\stdlib\*.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	      $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
else
CLEAN_FILES = *.o *.a misc/*.o libc/stdlib/*.o \
              $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
	      $(TARGETNAME).dll $(TARGETNAME).sym $(TARGETNAME).coff
endif

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

#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

# EOF
