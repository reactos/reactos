PATH_TO_TOP = ../../../..
TARGET_TYPE = driver
TARGET_NAME = pcnet

#
# - must define NDIS50 to get the right characteristics struct
# - must define anonymous unions to make physical addresses work right
#
TARGET_CFLAGS = -I. -Wall -Werror -DNDIS50 -DANONYMOUSUNIONS

# TARGET_CFLAGS += -I$(W32API_PATH)/include/ddk -D__USE_W32API

TARGET_OBJECTS = pcnet.o requests.o
TARGET_DDKLIBS = ndis.a
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
