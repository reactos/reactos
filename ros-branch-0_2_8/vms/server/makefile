# $Id$
#
# ReactOS VMS+ Environment Subsystem Server
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = native

TARGET_NAME = vmsss

TARGET_SDKLIBS = ntdll.a smdll.a

TARGET_CFLAGS = -Iinclude -I../include

TARGET_OBJECTS = vmsss.o init.o server.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
