PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = mpr

TARGET_BASE = 0x701a0000

TARGET_CFLAGS = -fno-builtin

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -DUNICODE -D_UNICODE -Wall -Werror

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = kernel32.a ntdll.a user32.a advapi32.a ole32.a

TARGET_OBJECTS = \
	../Uuid/uuid.o \
	auth.o \
	multinet.o \
	nps.o \
	pwcache.o \
	wnet.o \
	
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
