# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = snmpapi

TARGET_BASE = $(TARGET_BASE_LIB_SNMPAPI)

TARGET_CFLAGS = -DUNICODE

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
