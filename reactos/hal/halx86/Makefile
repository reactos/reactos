# $Id: Makefile,v 1.1 2001/08/21 20:18:26 chorns Exp $

PATH_TO_TOP = ../..

#
# Build configuration
#
include $(PATH_TO_TOP)/rules.mak

#
# Global configuration
#
include $(TOOLS_PATH)/config.mk

TARGET_TYPE = hal

TARGET_BASENAME = hal

TARGET_DEFNAME = ../hal/hal

TARGET_LIBPATH = .

TARGET_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/ntoskrnl/include -D__ASM__

TARGET_CFLAGS = -I./include -I$(PATH_TO_TOP)/ntoskrnl/include


TARGET_NAME_UP = halx86up

TARGET_NAME_MP = halx86mp

ifeq ($(MP), 1)
TARGET_NAME = $(TARGET_NAME_MP)
else
TARGET_NAME = $(TARGET_NAME_UP)
endif

HAL_OBJECTS = \
	adapter.o \
	beep.o \
	bios32.o \
	bus.o \
	display.o \
	dma.o \
	drive.o \
	enum.o \
	fmutex.o \
	halinit.o \
	isa.o \
	kdbg.o \
	mbr.o \
	misc.o \
	mp.o \
	pci.o \
	perfcnt.o \
	portio.o \
	reboot.o \
	spinlock.o \
	sysbus.o \
	sysinfo.o \
	time.o \
	udelay.o

	#pwroff.o

HAL_UP = \
  $(HAL_OBJECTS) \
	irql.o  

HAL_MP = \
  $(HAL_OBJECTS) \
	mpsirql.o \
	mpsboot.o \
	mps.o

ifeq ($(MP), 1)
DEP_OBJECTS := $(HAL_MP)
else
DEP_OBJECTS := $(HAL_UP)
endif

TARGET_OBJECTS := $(DEP_OBJECTS) $(PATH_TO_TOP)/include/roscfg.h

# Note: Must be = and not := since $(DEP_FILES) is assigned a value below
TARGET_CLEAN = $(DEP_FILES) *.o *.dll

#
# Helper makefile
#
include $(TOOLS_PATH)/helper.mk

#
# Include automatic dependancy tracking
#
include $(TOOLS_PATH)/depend.mk

# EOF
