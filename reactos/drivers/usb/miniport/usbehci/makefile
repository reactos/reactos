PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = usbehci

TARGET_DDKLIBS = ntoskrnl.a usbport.a

TARGET_CFLAGS = -Werror -Wall -I$(PATH_TO_TOP)/ntoskrnl/include -D__USE_W32API

TARGET_OBJECTS = \
  usbehci.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
