# $Id: makefile,v 1.69 2003/07/12 12:45:29 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = subsystem

TARGET_NAME = win32k

TARGET_BASE = 0x0

TARGET_ENTRY = _DllMain@8

TARGET_DDKLIBS = freetype.a

FREETYPE_DIR = $(PATH_TO_TOP)/lib/freetype

include $(PATH_TO_TOP)/config

ifeq ($(DBG), 1)
CFLAGS_DBG := -g
else
CFLAGS_DBG :=
endif

TARGET_CFLAGS =\
 $(CFLAGS_DBG) \
 -I$(PATH_TO_TOP)/ntoskrnl/include \
 -I$(FREETYPE_DIR)/include \
 -DUNICODE \
 -Wall

TARGET_LFLAGS =\
 $(PATH_TO_TOP)/dk/nkm/lib/freetype.a \
 $(PATH_TO_TOP)/dk/w32/lib/rosrtl.a \
 --disable-stdcall-fixup

ENG_OBJECTS= eng/debug.o eng/mem.o eng/brush.o eng/bitblt.o eng/clip.o \
    eng/copybits.o eng/device.o eng/handle.o eng/lineto.o eng/paint.o \
    eng/palette.o eng/perfcnt.o eng/surface.o eng/xlate.o eng/transblt.o \
    eng/mouse.o eng/misc.o eng/nls.o eng/sort.o

MAIN_OBJECTS = main/dllmain.o main/svctabm.o

MISC_OBJECTS = misc/driver.o misc/error.o misc/math.o misc/object.o

LDR_OBJECTS = ldr/loader.o

NTUSER_OBJECTS = ntuser/class.o ntuser/guicheck.o ntuser/hook.o \
                 ntuser/message.o ntuser/msgqueue.o ntuser/stubs.o \
                 ntuser/userobj.o ntuser/window.o ntuser/winsta.o \
		 ntuser/input.o ntuser/keyboard.o ntuser/callback.o \
		 ntuser/winpos.o ntuser/painting.o ntuser/metric.o \
		 ntuser/windc.o ntuser/prop.o ntuser/scrollbar.o \
		 ntuser/timer.o ntuser/misc.o

OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/gdiobj.o objects/icm.o  \
                  objects/line.o objects/metafile.o objects/paint.o  \
                  objects/path.o objects/pen.o objects/print.o  \
                  objects/region.o objects/text.o objects/wingl.o \
                  objects/bezier.o objects/objconv.o objects/dib.o \
                  objects/palette.o objects/rect.o objects/polyfill.o \
                  objects/cursoricon.o

DIB_OBJECTS = dib/dib.o dib/dib1bpp.o dib/dib4bpp.o dib/dib8bpp.o dib/dib16bpp.o \
              dib/dib24bpp.o dib/dib32bpp.o

STUBS_OBJECTS = stubs/stubs.o

TARGET_OBJECTS = \
  $(ENG_OBJECTS) $(MAIN_OBJECTS) $(MISC_OBJECTS) $(LDR_OBJECTS) \
  $(NTUSER_OBJECTS) $(OBJECTS_OBJECTS) $(STUBS_OBJECTS) \
  $(MATH_OBJECTS) $(FLOAT_OBJECTS) $(DIB_OBJECTS)

TARGET_CLEAN = $(DEP_FILES) \
  dib/*.o \
  eng/*.o \
  ldr/*.o \
  main/*.o \
  misc/*.o \
  ntuser/*.o \
  objects/*.o \
  stubs/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
DEP_EXCLUDE_FILTER := main/svctabm.d
#include $(PATH_TO_TOP)/tools/depend.mk

main/svctabm.o: main/svctab.c

.dummy:

%/TAGS: .dummy
	etags $(@D)/\*.c -o $(@D)/TAGS

etags: TAGS
TAGS: main/TAGS eng/TAGS ldr/TAGS misc/TAGS objects/TAGS stubs/TAGS ntuser/TAGS dib/TAGS
	etags -i main/TAGS -i eng/TAGS -i ldr/TAGS -i misc/TAGS -i objects/TAGS -i stubs/TAGS -i ntuser/TAGS -i dib/TAGS

# EOF
