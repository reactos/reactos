# $Id: makefile,v 1.21 2000/06/16 07:22:36 jfilby Exp $
#
# WIN32K.SYS build spec
#

TARGET=win32k

BASE_CFLAGS = -I../../include

ENG_OBJECTS= eng/debug.o eng/mem.o eng/brush.o eng/bitblt.o eng/clip.o eng/copybits.o \
		 eng/device.o eng/handle.o eng/lineto.o eng/paint.o eng/palette.o \
		eng/surface.o eng/xlate.o
MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/driver.o
MATH_OBJECTS = math/acos.o math/acosh.o math/asin.o math/asinh.o math/atan.o math/atan2.o \
		math/atanh.o math/ceil.o math/cos.o math/cosh.o math/exp.o math/fabs.o \
		math/floor.o math/fmod.o math/frexp.o math/huge_val.o math/hypot.o \
		math/ldexp.o math/log.o math/log10.o math/modf.o math/pow.o \
		math/sin.o math/sinh.o math/sqrt.o math/tan.o \
		math/tanh.o math/stubs.o math/j0_y0.o math/j1_y1.o math/jn_yn.o \
		math/cabs.o math/ftol.o
FLOAT_OBJECTS = float/fpreset.o float/clearfp.o float/cntrlfp.o float/statfp.o float/logb.o\
		float/chgsign.o float/fpclass.o float/isnan.o float/nafter.o float/scalb.o\
		float/copysign.o
LDR_OBJECTS = ldr/loader.o
OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/gdiobj.o objects/icm.o  \
                  objects/line.o objects/metafile.o objects/paint.o  \
                  objects/path.o objects/pen.o objects/print.o  \
                  objects/region.o objects/text.o objects/wingl.o \
                  objects/bezier.o objects/objconv.o
FREETYPE_OBJECTS =	freetype/grfont.o

RESOURCE_OBJECT = $(TARGET).coff
STUBS_OBJECTS = stubs/stubs.o

OBJECTS = $(ENG_OBJECTS) $(MAIN_OBJECTS) $(MISC_OBJECTS) $(LDR_OBJECTS) $(OBJECTS_OBJECTS)  \
          $(RESOURCE_OBJECT) $(STUBS_OBJECTS) $(MATH_OBJECTS) $(FLOAT_OBJECTS) $(FREETYPE_OBJECTS)

all: $(TARGET).sys

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = eng\*.o main\*.o misc\*.o ldr\*.o stubs\*.o objects\*.o  \
              $(TARGET).coff $(TARGET).o $(TARGET).a  \
              junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sym
else
CLEAN_FILES = eng/*.o main/*.o ldr\*.o misc/*.o stubs/*.o objects/*.o  \
              $(TARGET).coff $(TARGET).o $(TARGET).a ldr/*.o \
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

