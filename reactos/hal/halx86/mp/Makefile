# $Id: Makefile,v 1.2 2004/12/04 17:22:47 gvg Exp $

PATH_TO_TOP = ../../..

VPATH = ../generic

default: all

#
# Build configuration
#
include $(PATH_TO_TOP)/rules.mak

#
# Global configuration
#
include $(TOOLS_PATH)/config.mk

TARGET_TYPE = hal

TARGET_DEFNAME = ../../hal/hal

TARGET_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/ntoskrnl/include -D__ASM__ -DMP

TARGET_CFLAGS = -I../include -I$(PATH_TO_TOP)/ntoskrnl/include -Wall -Werror -DMP

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_NAME = halmp

ifneq ($(MP), 1)
TARGET_INSTALL = no
else
TARGET_BOOTSTRAP = yes
endif

GENERIC_OBJECTS = \
	adapter.o \
	beep.o \
	bus.o \
	display.o \
	dma.o \
	drive.o \
	enum.o \
	fmutex.o \
	halinit.o \
	isa.o \
	kdbg.o \
	mca.o \
	mem.o \
	misc.o \
	pci.o \
	portio.o \
	reboot.o \
	spinlock.o \
	sysbus.o \
	sysinfo.o \
	time.o \
	timer.o

MP_OBJECTS = \
	apic.o \
	halinit_mp.o \
	ipi_mp.o \
	mpsirql.o \
	mpsboot.o \
	mps.o \
	processor_mp.o \
	resource_mp.o

HAL_OBJECTS = $(GENERIC_OBJECTS) $(MP_OBJECTS)

DEP_OBJECTS := $(HAL_OBJECTS)

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
