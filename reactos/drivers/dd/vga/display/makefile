# $Id: makefile,v 1.17 2001/07/26 16:52:07 ekohl Exp $
#
# Makefile for ReactOS vgaddi.dll
#
PATH_TO_TOP = ../../../..

TARGET = vgaddi

CFLAGS = -D__NTDRIVER__ -I.
 
MAIN_OBJECTS = main/enable.o
OTHER_OBJECTS = objects/screen.o objects/pointer.o objects/lineto.o objects/paint.o objects/bitblt.o \
		vgavideo/vgavideo.o objects/transblt.o
RESOURCE_OBJECTS = $(TARGET).coff

OBJECTS = $(MAIN_OBJECTS) $(OTHER_OBJECTS) $(RESOURCE_OBJECTS)

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a \
       $(PATH_TO_TOP)/subsys/win32k/win32k.a


all: $(TARGET).dll $(TARGET).a

$(TARGET).a: $(OBJECTS)
	$(AR) csr $(TARGET).a $(OBJECTS)

$(TARGET).coff: $(TARGET).rc $(PATH_TO_TOP)/include/reactos/resource.h

$(TARGET).dll: $(OBJECTS) $(LIBS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--kill-at \
		--output-lib $(TARGET).a
	$(CC) $(TARGET).o \
		$(LIBS) \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--entry,_DrvEnableDriver \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf
	- $(RM) base.tmp
	$(CC) $(TARGET).o \
		$(LIBS) \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o $(TARGET).dll \
		-Wl,--subsystem,native \
		-Wl,--entry,_DrvEnableDriver \
		-Wl,--image-base,0x70000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

CLEAN_FILES = *.o *.coff *.sym *.tmp *.dll main/*.o objects/*.o vgavideo/*.o *.a

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(PATH_TO_TOP)/$(DIST_DIR)/dlls/$(TARGET).dll

include $(PATH_TO_TOP)/rules.mak

# EOF
