# $Id$

PATH_TO_TOP = ../../..
TARGET_BOOTSTRAP = yes
TARGET_TYPE = driver
TARGET_NAME = ext2
TARGET_GCCLIBS = gcc
TARGET_CFLAGS = -Iinc -I$(W32API_PATH)/include/ddk -D__USE_W32API \
                -D_CPU_X86_ -O2 -DDBG=1 -D__REACTOS__

# FIXME: Enabling this reveals that there is quite a few pieces of unused
#        code.
#TARGET_CFLAGS += -Wall

# FIXME: Do not b*tch about multi-character constants used for TAGs. Use
#        the TAG macro instead?
TARGET_CFLAGS += -Wno-multichar

# FIXME: Force STDCALL as default calling convention. Due bug in GCC it
#        it still produces non-STDCALL function if an function declaration
#        preceeds the actual function definition and doesn't explicitely
#        specify calling convention.
TARGET_CFLAGS += -mrtd

# FIXME: Until PSEH and MS2PS will get up to speed we'll ignore SEH.
TARGET_CFLAGS += "-Dtry=if(1)" "-Dexcept(x)=if(0)" "-Dfinally=if(1)"

TARGET_OBJECTS = \
	src/cleanup.o \
	src/close.o \
	src/create.o \
	src/devcntrl.o \
	src/dircntrl.o \
	src/DiskIO.o \
	src/ext2init.o \
	src/fastio.o \
	src/fileinfo.o \
	src/flush.o \
	src/fsctrl.o \
	src/io.o \
	src/metadata.o \
	src/misc.o \
	src/read.o \
	src/shutdown.o \
	src/volinfo.o \
	src/write.o
TARGET_RC_SRCS = src/ext2.rc

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/config
include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk
include $(TOOLS_PATH)/depend.mk
