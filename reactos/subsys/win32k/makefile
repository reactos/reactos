# $Id: makefile,v 1.41 2002/01/13 22:52:08 dwelch Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = export_driver

TARGET_NAME = win32k

TARGET_BASE = 0x0

TARGET_ENTRY = _DllMain@8

# from atheos appserver makefile
COPTS   = -pipe -O3 -I./freetype/include -c -Wall

TARGET_CFLAGS = -I$(PATH_TO_TOP)/ntoskrnl/include -DUNICODE -Wall

ENG_OBJECTS= eng/debug.o eng/mem.o eng/brush.o eng/bitblt.o eng/clip.o \
    eng/copybits.o eng/device.o eng/handle.o eng/lineto.o eng/paint.o \
    eng/palette.o eng/surface.o eng/xlate.o eng/transblt.o eng/mouse.o
MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/driver.o misc/error.o misc/math.o misc/object.o
LDR_OBJECTS = ldr/loader.o
NTUSER_OBJECTS = ntuser/class.o ntuser/guicheck.o ntuser/hook.o \
                 ntuser/message.o ntuser/msgqueue.o ntuser/stubs.o \
                 ntuser/userobj.o ntuser/window.o ntuser/winsta.o \
		 ntuser/input.o
OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/gdiobj.o objects/icm.o  \
                  objects/line.o objects/metafile.o objects/paint.o  \
                  objects/path.o objects/pen.o objects/print.o  \
                  objects/region.o objects/text.o objects/wingl.o \
                  objects/bezier.o objects/objconv.o objects/dib.o \
	          objects/palette.o
DIB_OBJECTS = dib/dib4bpp.o dib/dib24bpp.o
FREETYPE_OBJECTS = freetype/ctype.o freetype/grfont.o \
		freetype/src/base/ftsystem.o freetype/src/base/ftdebug.o \
		freetype/src/base/ftinit.o freetype/src/base/ftbase.o \
		freetype/src/base/ftglyph.o freetype/src/base/ftmm.o \
		freetype/src/autohint/autohint.o freetype/src/cff/cff.o \
		freetype/src/cid/type1cid.o freetype/src/psnames/psnames.o \
		freetype/src/winfonts/winfnt.o freetype/src/raster1/raster1.o \
		freetype/src/sfnt/sfnt.o freetype/src/smooth/smooth.o \
		freetype/src/truetype/truetype.o freetype/src/type1z/type1z.o
STUBS_OBJECTS = stubs/stubs.o

TARGET_OBJECTS = \
  $(ENG_OBJECTS) $(MAIN_OBJECTS) $(MISC_OBJECTS) $(LDR_OBJECTS) \
  $(NTUSER_OBJECTS) $(OBJECTS_OBJECTS) $(STUBS_OBJECTS) \
  $(MATH_OBJECTS) $(FLOAT_OBJECTS) $(FREETYPE_OBJECTS) $(DIB_OBJECTS)

TARGET_CLEAN = \
  dib/*.o \
  eng/*.o \
  freetype/*.o \
  ldr/*.o \
  main/*.o \
  misc/*.o \
  ntuser/*.o \
  objects/*.o \
  stubs/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

.dummy:

%/TAGS: .dummy
	etags $(@D)/\*.c -o $(@D)/TAGS

etags: TAGS
TAGS: main/TAGS eng/TAGS ldr/TAGS misc/TAGS objects/TAGS stubs/TAGS ntuser/TAGS dib/TAGS freetype/TAGS
	etags -i main/TAGS -i eng/TAGS -i ldr/TAGS -i misc/TAGS -i objects/TAGS -i stubs/TAGS -i ntuser/TAGS -i dib/TAGS -i freetype/TAGS

# EOF
