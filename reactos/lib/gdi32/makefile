# $Id: makefile,v 1.21 2002/08/18 07:02:56 ei Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = gdi32

TARGET_BASE = 0x77ed0000

TARGET_SDKLIBS = ntdll.a kernel32.a advapi32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = main/*.o misc/*.o objects/*.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

MAIN_OBJECTS = main/dllmain.o

MISC_OBJECTS = misc/stubs.o misc/stubsa.o misc/stubsw.o misc/win32k.o

OBJECTS_OBJECTS = objects/dc.o objects/line.o objects/pen.o objects/bitblt.o objects/text.o objects/region.o objects/brush.o

OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(OBJECTS_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
