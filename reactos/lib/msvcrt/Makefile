# $Id: Makefile,v 1.5 2000/11/20 19:59:08 ekohl Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET_NAME=msvcrt
TARGET_DLL=$(TARGET_NAME).dll

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
		-Wl,--image-base,0x78000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET_NAME).dll > $(TARGET_NAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

install: $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll

$(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
	$(CP) $(TARGET_NAME).dll $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll

dist: $(DIST_DIR)/dlls/$(TARGET_NAME).dll

$(DIST_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
	$(CP) $(TARGET_NAME).dll ../../$(DIST_DIR)/dlls/$(TARGET_NAME).dll

include ../../rules.mak

