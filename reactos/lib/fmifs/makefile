# $Id: makefile,v 1.7 1999/08/29 13:44:53 dwelch Exp $
#
# Makefile for fmifs.dll
#
TARGET=fmifs
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=$(TARGET).a
  else
    DLLTARGET=$(TARGET).dll
  endif
else
  DLLTARGET=$(TARGET).dll
endif

BASE_CFLAGS = -I../../include

all: $(DLLTARGET)

IFS_OBJECTS = chkdsk.o compress.o diskcopy.o extend.o \
	format.o init.o label.o media.o 
	
MISC_OBJECTS = dllmain.o $(TARGET).coff

OBJECTS = $(IFS_OBJECTS) $(MISC_OBJECTS)

CLEAN_FILES = *.o *.a *.tmp *.dll *.sym *.coff

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
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
		--def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) \
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		$(TARGET).o \
		-Wl,--image-base,0x70000000 \
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

include ../../rules.mak

# EOF
