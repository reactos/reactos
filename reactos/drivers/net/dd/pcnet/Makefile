PATH_TO_TOP = ../../../..
TARGET_TYPE = driver
TARGET_NAME = pcnet

#
# - must define NDIS50 to get the right characteristics struct
# - must define anonymous unions to make physical addresses work right
#
TARGET_CFLAGS = -I. -DDBG=0 -Wall -Werror -DNDIS50 -DANONYMOUSUNIONS 

TARGET_OBJECTS = pcnet.o requests.o
TARGET_DDKLIBS = ndis.a
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
