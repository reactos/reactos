# $Id: Makefile,v 1.35 2004/02/16 01:30:38 rcampbell Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = user32

TARGET_BASE = 0x77e70000

TARGET_SDKLIBS = libwine.a ntdll.a kernel32.a gdi32.a rosrtl.a advapi32.a

TARGET_CFLAGS = \
 -I./include \
 -DUNICODE \
 -D__REACTOS__ \
 -Wall \
 -Werror \
 -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_RCFLAGS = --include-dir ./include

CONTROLS_OBJECTS = \
	controls/combo.o \
	controls/edit.o \
	controls/icontitle.o \
	controls/listbox.o \
	controls/scrollbar.o \
	controls/static.o \
	controls/button.o \
	controls/regcontrol.o

MISC_OBJECTS = \
	misc/dde.o \
	misc/ddeclient.o \
	misc/ddeserver.o \
	misc/desktop.o \
	misc/display.o \
	misc/dllmain.o \
	misc/exit.o \
	misc/exticon.o \
	misc/wsprintf.o \
	misc/stubs.o \
	misc/winhelp.o \
	misc/win32k.o \
	misc/winsta.o \
	misc/resources.o \
	misc/object.o \
	misc/timer.o \
	misc/strpool.o \
	misc/misc.o

WINDOWS_OBJECTS = \
	windows/caret.o \
	windows/class.o \
	windows/message.o \
	windows/window.o \
	windows/defwnd.o \
	windows/accel.o \
	windows/bitmap.o \
	windows/clipboard.o \
	windows/cursor.o \
	windows/dc.o \
	windows/dialog.o \
	windows/draw.o \
	windows/font.o \
	windows/hook.o \
	windows/icon.o \
	windows/input.o \
	windows/mdi.o \
	windows/menu.o \
	windows/messagebox.o \
	windows/nonclient.o \
	windows/paint.o \
	windows/prop.o \
	windows/rect.o \
	windows/text.o \
	windows/winpos.o

TARGET_OBJECTS = \
  $(MISC_OBJECTS) \
  $(WINDOWS_OBJECTS) \
  $(CONTROLS_OBJECTS)

DEP_OBJECTS = $(TARGET_OBJECTS)

DEP_EXCLUDE_FILTER = misc/win32k.%

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

include $(TOOLS_PATH)/config.mk

# EOF
