# $Id: makefile,v 1.3 1999/05/24 20:04:44 ea Exp $
#
# Makefile for ReactOS gdi32.dll
#
TARGET=gdi32

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

MAIN_OBJECTS = main/dllmain.o

MISC_OBJECTS = misc/stubs.o misc/stubsa.o misc/stubsw.o

RESOURCE_OBJECTS = $(TARGET).coff

OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(RESOURCE_OBJECTS)

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).coff: $(TARGET).rc
	$(RC) $(TARGET).rc $(TARGET).coff

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--output-lib $(TARGET).a
	$(CC) $(TARGET).o \
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
		--def $(TARGET).def
	- $(RM) base.tmp
	$(CC) $(TARGET).o \
		../kernel32/kernel32.a \
		-specs=$(TARGET)_specs \
		-mdll \
		-o $(TARGET).dll \
		-Wl,--image-base,0x70000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

ifeq ($(DOSCLI),yes)
CLEAN_FILES = *.o *.coff *.sym *.tmp *.dll main\*.o misc\*.o
else
CLEAN_FILES = *.o *.coff *.sym *.tmp *.dll main/*.o misc/*.o
endif

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)


include ../../rules.mak

# EOF
