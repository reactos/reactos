#
#  ReactOS Console TaskManager
#
#  Makefile
#

PATH_TO_TOP = ../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = ctm

TARGET_SDKLIBS = epsapi.a ntdll.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -D__USE_W32API -DUNICODE -D_UNICODE

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
