
PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = dflat32

TARGET_LFLAGS = 

TARGET_SDKLIBS = 

TARGET_GCCLIBS =

TARGET_BASE = 0x98000000

TARGET_CFLAGS = # -pipe -O2 -Wall -Wstrict-prototypes -fno-builtin  -DDBG

TARGET_OBJECTS = \
clipbord.o \
config.o \
console.o \
dllmain.o \
dfalloc.o \
message.o \
rect.o \
stubs.o \
video.o \
window.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
