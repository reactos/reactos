PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = ddraw

# -fno-builtin
TARGET_CFLAGS = -D__USE_W32API -I$(PATH_TO_TOP)/include/wine

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS -Wall 

TARGET_SDKLIBS = ntdll.a gdi32.a dxguid.a ole32.a user32.a d3d8thk.a


TARGET_OBJECTS = \
      regsvr.o \
      ddraw.o \
      ddraw_hal.o 
     


DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
