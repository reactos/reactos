PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = opengl32

# -fno-builtin
TARGET_CFLAGS = -D__USE_W32API

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_SDKLIBS = ntdll.a gdi32.a

TARGET_OBJECTS = \
	opengl32.o \
	wgl.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
