#
#  ReactOS calc
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

WINE_MODE = yes

WINE_RC = calc

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = calc

WINE_INCLUDE = ./

TARGET_CFLAGS = -D__USE_W32API

#TARGET_CFLAGS = -I$(TEST_ROOT) -DDBG -DUNICODE -D_UNICODE

BASE_CFLAGS = -D_WIN32_IE=0x0400

RCFLAGS = -D_WIN32_IE=0x0400

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a

TARGET_GCCLIBS = comctl32

TARGET_OBJECTS = \
	settings.o \
        button.o \
        main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
