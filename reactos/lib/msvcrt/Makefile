# $Id: Makefile,v 1.1 2000/06/18 10:57:42 ea Exp $
#
# ReactOS Operating System
#
TARGET_NAME=msvcrt
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    TARGET_DLL=$(TARGET_NAME).a
  else
    TARGET_DLL=$(TARGET_NAME).dll
  endif
else
  TARGET_DLL=$(TARGET_NAME).dll
endif

BASE_CFLAGS = -I../../include

all: $(TARGET_DLL)



OBJECTS_MISC = \
	misc/dllmain.o \
	misc/msvcrt.coff

OBJECTS_PROCESS = \
	process/thread.o

OBJECTS_STDLIB = \
	stdlib/errno.o


OBJECTS = \
	$(OBJECTS_MISC) \
	$(OBJECTS_PROCESS) \
	$(OBJECTS_STDLIB)

ifeq ($(DOSCLI), yes)
CLEAN_FILES = \
	misc\*.o \
	misc\*.coff \
	process\*.o \
	stdlib\*.o \
	$(TARGET_NAME).a \
	$(TARGET_NAME).o \
	$(TARGET_NAME).dll \
	$(TARGET_NAME).sym
else
CLEAN_FILES = \
	misc/*.o \
	misc/*.coff \
	process/*.o \
	stdlib/*.o \
	$(TARGET_NAME).a \
	$(TARGET_NAME).o \
	$(TARGET_NAME).dll \
	$(TARGET_NAME).sym
endif

$(TARGET_NAME).coff: $(TARGET_NAME).rc ../../include/reactos/resource.h

$(TARGET_NAME).a: $(OBJECTS)
	$(LD)  -r $(OBJECTS) -o $(TARGET_NAME).a

$(TARGET_NAME).dll: $(DLLMAIN) $(OBJECTS) misc/$(TARGET_NAME).def
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o
	$(DLLTOOL) \
		--dllname $(TARGET_NAME).dll \
		--def misc/$(TARGET_NAME).def \
		--output-lib misc/$(TARGET_NAME).a
	$(CC) \
		-specs=misc/$(TARGET_NAME)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGET_NAME).o \
		../kernel32/kernel32.a 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET_NAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def misc/$(TARGET_NAME).def
	- $(RM) base.tmp
	$(CC) \
		-specs=misc/$(TARGET_NAME)_specs \
		-mdll \
		-o $(TARGET_NAME).dll \
		$(TARGET_NAME).o  \
		../kernel32/kernel32.a \
		-Wl,--image-base,0x77630000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET_NAME).dll > $(TARGET_NAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

floppy: $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll

$(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET_NAME).dll $(FLOPPY_DIR)\dlls\$(TARGET_NAME).dll
else
	$(CP) $(TARGET_NAME).dll $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll
endif

dist: $(DIST_DIR)/dlls/$(TARGET_NAME).dll

$(DIST_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET_NAME).dll ..\..\$(DIST_DIR)\dlls\$(TARGET_NAME).dll
else
	$(CP) $(TARGET_NAME).dll ../../$(DIST_DIR)/dlls/$(TARGET_NAME).dll
endif

include ../../rules.mak

