
TARGET_TYPE = dynlink
TARGET_NAME = package
PATH_TO_TOP = ../../../reactos
TARGET_INSTALLDIR = bin

TARGET_SDKLIBS = kernel32.a shell32.a user32.a package.a expat.a urlmon.a

TARGET_OBJECTS = main.o tree.o package.o download.o script.o functions.o options.o log.o

TARGET_GCCLIBS = stdc++ uuid


TARGET_CFLAGS = \
 -DUNICODE \
 -Wall \
 -Wno-error \
 -D__USE_W32API \
 -DWINVER=0x0600 \
 -D_WIN32_WINNT=0x0501

TARGET_CPPFLAGS = $(TARGET_CFLAGS)

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk

# EOF
