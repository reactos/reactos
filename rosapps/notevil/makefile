#
#  ReactOS notevil
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = notevil

#TARGET_CFLAGS = -DUNICODE -D_UNICODE

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
