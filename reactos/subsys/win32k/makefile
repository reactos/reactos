# $Id: makefile,v 1.37 2001/07/15 21:19:02 rex Exp $
#
# WIN32K.SYS build spec
#
PATH_TO_TOP = ../..

TARGET=win32k

# from atheos appserver makefile
COPTS   = -pipe -O3 -I./freetype/include -c -Wall

CFLAGS = -D__NTDRIVER__ -I. -I$(PATH_TO_TOP)/ntoskrnl/include -DUNICODE

#define FT_FLAT_COMPILE

ENG_OBJECTS= eng/debug.o eng/mem.o eng/brush.o eng/bitblt.o eng/clip.o eng/copybits.o \
		 eng/device.o eng/handle.o eng/lineto.o eng/paint.o eng/palette.o \
		eng/surface.o eng/xlate.o eng/transblt.o eng/mouse.o
MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/driver.o misc/error.o misc/math.o misc/object.o
LDR_OBJECTS = ldr/loader.o
NTUSER_OBJECTS = ntuser/class.o ntuser/guicheck.o ntuser/hook.o \
                 ntuser/message.o ntuser/msgqueue.o ntuser/stubs.o \
                 ntuser/userobj.o ntuser/window.o ntuser/winsta.o
OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/gdiobj.o objects/icm.o  \
                  objects/line.o objects/metafile.o objects/paint.o  \
                  objects/path.o objects/pen.o objects/print.o  \
                  objects/region.o objects/text.o objects/wingl.o \
                  objects/bezier.o objects/objconv.o objects/dib.o objects/palette.o
DIB_OBJECTS = dib/dib4bpp.o dib/dib24bpp.o
FREETYPE_OBJECTS = freetype/grfont.o freetype/ctype.o \
	freetype/ftsystem.o freetype/ftdebug.o freetype/ftinit.o freetype/ftbase.o \
	freetype/ftglyph.o freetype/ftmm.o freetype/autohint.o freetype/cff.o \
	freetype/type1cid.o freetype/psnames.o freetype/raster1.o freetype/sfnt.o \
	freetype/smooth.o freetype/truetype.o freetype/winfnt.o freetype/type1z.o
RESOURCE_OBJECT = $(TARGET).coff
STUBS_OBJECTS = stubs/stubs.o

OBJECTS = $(ENG_OBJECTS) $(MAIN_OBJECTS) $(MISC_OBJECTS) $(LDR_OBJECTS) \
          $(NTUSER_OBJECTS) $(OBJECTS_OBJECTS) $(RESOURCE_OBJECT) \
          $(STUBS_OBJECTS) $(MATH_OBJECTS) $(FLOAT_OBJECTS) \
          $(FREETYPE_OBJECTS) $(DIB_OBJECTS)

all: $(TARGET).sys

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

CLEAN_FILES = dib/*.o eng/*.o freetype/*.o ldr/*.o main/*.o misc/*.o ntuser/*.o  \
              objects/*.o stubs/*.o  \
              $(TARGET).coff $(TARGET).o $(TARGET).a  \
              junk.tmp base.tmp temp.exp $(TARGET).sys $(TARGET).sym

$(TARGET).sys: $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).sys \
		--def $(TARGET).def \
		--output-lib $(TARGET).a \
		--kill-at
	$(CC) \
		-nostartfiles -nostdlib \
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
		-nostartfiles -nostdlib \
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
		-Wl,temp.exp 
              
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).sys > $(TARGET).sym

clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(DIST_DIR)/drivers/$(TARGET).sys

$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys ../../$(DIST_DIR)/drivers/$(TARGET).sys

#WITH_DEBUGGING = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak
