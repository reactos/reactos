#
#  ReactOS notepad
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = notepad

TARGET_CFLAGS = -D_WIN32_IE=0x0400 # -DUNICODE -D_UNICODE

TARGET_RCFLAGS = -D_WIN32_IE=0x0400

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	$(COMCTL32_TARGET).a \
	$(COMDLG32_TARGET).a

TARGET_OBJECTS = \
	framewnd.o \
        main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
