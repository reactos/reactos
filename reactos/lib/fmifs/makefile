# $Id: makefile,v 1.3 1999/06/18 22:11:01 ea Exp $
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
		--output-lib $(TARGET).a \
		--add-stdcall-alias \
		--kill-at
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
		--def $(TARGET).def \
		--add-stdcall-alias \
		--kill-at
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

include ../../rules.mak

# EOF
