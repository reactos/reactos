PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = opengl32

# -fno-builtin
TARGET_CFLAGS = -D__USE_W32API -DUNICODE -D_M_IX86 -Os -g3

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall -Werror

TARGET_LFLAGS = -lgdi32 -lntdll

TARGET_SDKLIBS = gdi32.a ntdll.a
# ntdll.a gdi32.a - may still need these

TARGET_OBJECTS = \
	gl.o \
	opengl32.o \
	wgl.o \
	#tebimports.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
