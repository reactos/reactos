# $Id: makefile,v 1.18 2000/03/11 00:51:36 ea Exp $
#
# WIN32K.SYS build spec
#

TARGET=win32k

BASE_CFLAGS = -I../../include

ENG_OBJECTS= eng/debug.o eng/mem.o
MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/driver.o
LDR_OBJECTS = ldr/loader.o
OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/gdiobj.o objects/icm.o  \
                  objects/line.o objects/metafile.o objects/paint.o  \
                  objects/path.o objects/pen.o objects/print.o  \
                  objects/region.o objects/text.o objects/wingl.o
RESOURCE_OBJECT = $(TARGET).coff
STUBS_OBJECTS = stubs/stubs.o

OBJECTS = $(ENG_OBJECTS) $(MAIN_OBJECTS) $(MISC_OBJECTS) $(LDR_OBJECTS) $(OBJECTS_OBJECTS)  \
          $(RESOURCE_OBJECT) $(STUBS_OBJECTS) 

all: $(TARGET).sys

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = eng\*.o main\*.o misc\*.o ldr\*.o stubs\*.o objects\*.o  \
              $(TARGET).coff $(TARGET).o $(TARGET).a  \
              junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sym
else
CLEAN_FILES = eng/*.o main/*.o ldr\*.o misc/*.o stubs/*.o objects/*.o  \
              $(TARGET).coff $(TARGET).o $(TARGET).a  \
              junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sym
endif

$(TARGET).sys: $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--def $(TARGET).def \
		--output-lib $(TARGET).a \
		--kill-at
	$(CC) \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DllMain@8 \
		-o junk.tmp \
		-Wl,--image-base,0x0 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $(TARGET).o \
		-specs=../../specs \
		../../ntoskrnl/ntoskrnl.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DllMain@8 \
		-o $(TARGET).sys \
		$(TARGET).o \
		../../ntoskrnl/ntoskrnl.a \
		-Wl,--image-base,0x0 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,temp.exp \
		-specs=../../specs
              
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).sys > $(TARGET).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

floppy: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys $(FLOPPY_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys
endif

dist: $(DIST_DIR)/drivers/$(TARGET).sys

$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGET).sys ..\..\$(DIST_DIR)\drivers\$(TARGET).sys
else
	$(CP) $(TARGET).sys ../../$(DIST_DIR)/drivers/$(TARGET).sys
endif

#WITH_DEBUGGING = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

