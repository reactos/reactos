# $Id: makefile,v 1.5 2001/08/21 20:13:07 chorns Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = shell32

TARGET_BASE = 0x77260000

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = misc/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

MISC_OBJECTS = \
	misc/dllmain.o \
	misc/stubs.o 

OBJECTS = \
  $(MISC_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
