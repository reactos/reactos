# $Id$

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = crtdll

TARGET_PCH = precomp.h

TARGET_BASE = $(TARGET_BASE_LIB_CRTDLL)

# don't remove @nn from exported symbols - needed so dlltool doesn't mess up mangled c++ exports
RM_AT_FROM_SYMBOLS = no

TARGET_LFLAGS = -nostartfiles -nostdlib --enable-stdcall-fixup

TARGET_SDKLIBS = crt.a string.a kernel32.a ntdll.a wine.a

TARGET_GCCLIBS = gcc

TARGET_CFLAGS = -D__USE_W32API -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += \
	-D_DISABLE_TIDENTS \
	-D__USE_W32API \
	-D__REACTOS__ \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 \
	-DUSE_MSVCRT_PREFIX \
	-D_MT

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = dllmain.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk




OBJECTS = 	dllmain.o

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o


# EOF
