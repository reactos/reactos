PATH_TO_TOP = ../..

TARGET_REGTESTS = yes

TARGET_TYPE = dynlink

TARGET_NAME = gdiplus

TARGET_BASE = $(TARGET_BASE_LIB_GDIPLUS)

TARGET_SDKLIBS = ntdll.a kernel32.a gdi32.a msvcrt.a

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -Wall \
 -Werror \
 -fno-builtin \
 -D__USE_W32API \
 -DWINVER=0x0600 \
 -D_WIN32_WINNT=0x0501

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

GDIPLUS_OBJECTS = \
  gdiplus/arrow.o \
  gdiplus/bitmap.o \
  gdiplus/brush.o \
  gdiplus/clip.o \
  gdiplus/codec.o \
  gdiplus/container.o \
  gdiplus/dllmain.o \
  gdiplus/draw.o \
  gdiplus/effect.o \
  gdiplus/fill.o \
  gdiplus/font.o \
  gdiplus/graphics.o \
  gdiplus/image.o \
  gdiplus/linecap.o \
  gdiplus/linegradient.o \
  gdiplus/matrix.o \
  gdiplus/memory.o \
  gdiplus/metafile.o \
  gdiplus/palette.o \
  gdiplus/path.o \
  gdiplus/pathgradient.o \
  gdiplus/pathiterator.o \
  gdiplus/pen.o \
  gdiplus/region.o \
  gdiplus/string.o \
  gdiplus/texture.o \
  gdiplus/transform.o

TARGET_OBJECTS = $(GDIPLUS_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
