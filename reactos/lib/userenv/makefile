PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = userenv

TARGET_BASE = 0x74850000

TARGET_CFLAGS = -fno-builtin -D__USE_W32API -D_WIN32_IE=0x0400

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -DUNICODE -D_UNICODE -Wall -Werror

TARGET_LFLAGS = -nostdlib -nostartfiles

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a ole32.a wine_uuid.a

TARGET_OBJECTS = desktop.o directory.o environment.o profile.o misc.o \
                 registry.o setup.o userenv.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
