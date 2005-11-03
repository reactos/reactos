# $Id: Makefile,v 1.1 2003/09/27 18:52:31 guido Exp $
#
# PSXX.DLL
#

PATH_TO_TOP = ../../../reactos

PATH_TO_PSX_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psxx

#TARGET_LIBPATH = $(PATH_TO_PSX_TOP)/dk/lib

TARGET_SDKLIBS = ntdll.a

#TARGET_BASE = 0x77dc0000

TARGET_ENTRY = _DllMain@12

TARGET_CFLAGS = -D__PSXX__ -D__PSXDLL__

TARGET_LFLAGS = \
	-Wl,--file-alignment,0x1000 \
	-Wl,--section-alignment,0x1000 \
	-Wl,--subsystem,posix:19.90


TARGET_PATH = misc


MISC_OBJECTS = \
	misc/$(TARGET_NAME).coff \
	misc/main.o \
	misc/stubs.o
	
TARGET_OBJECTS = \
	$(MISC_OBJECTS)

#TARGET_CLEAN = \
#  misc/*.o \
#  misc/*.coff

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

#DTFLAGS = -k -l $@

#$(TARGET_NAME).a: $(TARGET_PATH)/$(TARGET_NAME).def
#	$(DLLTOOL) \
#		$(DTFLAGS) \
#		-D $(TARGET_NAME).dll \
#		-d $(TARGET_PATH)/$(TARGET_NAME).def

# EOF
