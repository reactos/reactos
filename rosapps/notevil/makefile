#
#  ReactOS notevil
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = notevil

TARGET_CFLAGS = -D__USE_W32API #-DUNICODE -D_UNICODE

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

$(TARGET_NAME).o: resource.h

# EOF
