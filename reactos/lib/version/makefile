# $Id: makefile,v 1.7 2001/07/15 21:18:56 rex Exp $
#
# Makefile for ReactOS version.dll
#
PATH_TO_TOP = ../..

TARGET=version

BASE_CFLAGS = -I../../include

DLLTARGET=$(TARGET).dll

all: $(DLLTARGET)

MISC_OBJECTS = misc/libmain.o misc/stubs.o

RESOURCE_OBJECTS = $(TARGET).coff

OBJECTS = $(MISC_OBJECTS) $(RESOURCE_OBJECTS)

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
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		-Wl,--entry,_DllMain@12 \
		-Wl,--image-base,0x77a90000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

CLEAN_FILES = *.o *.a *.coff *.sym *.tmp *.dll main/*.o misc/*.o

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll ../../$(DIST_DIR)/dlls/$(TARGET).dll

include ../../rules.mak

# EOF
