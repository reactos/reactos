# $Id: makefile,v 1.15 2000/08/18 22:27:00 dwelch Exp $
#
# Makefile for ReactOS gdi32.dll
#
PATH_TO_TOP = ../..

TARGET=gdi32

BASE_CFLAGS = -I../../include

DLLTARGET=$(TARGET).dll


all: $(DLLTARGET)

MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/stubs.o misc/stubsa.o misc/stubsw.o misc/win32k.o
RESOURCE_OBJECTS = $(TARGET).coff
OBJECTS_OBJECTS = objects/dc.o objects/line.o objects/pen.o objects/bitblt.o objects/text.o

OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(RESOURCE_OBJECTS) $(OBJECTS_OBJECTS)

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) $(TARGET).o \
		../kernel32/kernel32.a \
		../ntdll/ntdll.a \
		-specs=$(TARGET)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--entry,_DllMain@12 \
		-Wl,--base-file,base.tmp 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) $(TARGET).o \
		../kernel32/kernel32.a \
		../ntdll/ntdll.a \
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		-Wl,--entry,_DllMain@12 \
		-Wl,--image-base,0x70000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

ifeq ($(DOSCLI),yes)
CLEAN_FILES = *.a *.o *.coff *.sym *.tmp *.dll main\*.o misc\*.o objects\*.o
else
CLEAN_FILES = *.a *.o *.coff *.sym *.tmp *.dll main/*.o misc/*.o objects/*.o
endif 

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

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

include ../../rules.mak

# EOF
