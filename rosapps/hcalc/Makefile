#
#  ReactOS hcalc
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = hcalc

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a

TARGET_OBJECTS = \
	hcalc.o \
        input.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
