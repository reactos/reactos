
PATH_TO_TOP = ../../..

TARGET_NORC = yes

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = enumfonts

TARGET_SDKLIBS = kernel32.a gdi32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_GCCLIBS = stdc++

TARGET_CFLAGS = -Wall -Werror -D__USE_W32API -DWIN32 -D_WIN32_IE=0x0600 -D_WIN32_WINNT=0x0501 -fexceptions -Wall -I.

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
