# $Id: Makefile,v 1.6 2002/09/17 23:46:22 dwelch Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = user32

TARGET_BASE = 0x77e70000

TARGET_SDKLIBS = ntdll.a kernel32.a gdi32.a

TARGET_CFLAGS = -I./include -DUNICODE -DDBG -Wall -Werror

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = misc/*.o windows/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

CONTROLS_OBJECTS = \
	controls/combobox.o \
	controls/listbox.o \
	controls/scrollbar.o

MISC_OBJECTS = \
	misc/desktop.o \
	misc/dllmain.o \
	misc/sprintf.o \
	misc/stubs.o \
	misc/winhelp.o \
	misc/win32k.o \
	misc/winsta.o \
	misc/resources.o \
	misc/dde.o \
	misc/display.o \
	misc/object.o \
	misc/timer.o

WINDOWS_OBJECTS = \
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
	windows/text.o

OBJECTS = \
  $(MISC_OBJECTS) \
  $(WINDOWS_OBJECTS) \
  $(CONTROLS_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
