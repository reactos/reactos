PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = usbport

TARGET_DDKLIBS = ntoskrnl.a

TARGET_CFLAGS = -Werror -Wall -I$(PATH_TO_TOP)/ntoskrnl/include -D__USE_W32API

TARGET_OBJECTS = \
  usbport.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
