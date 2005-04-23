# $Id$

PATH_TO_TOP = ../../..

VPATH = ../generic

default: all

TARGET_NAME = halxbox

#
# Build configuration
#
include $(PATH_TO_TOP)/rules.mak

#
# Global configuration
#
include $(TOOLS_PATH)/config.mk

TARGET_BOOTSTRAP = yes

TARGET_TYPE = hal

TARGET_DEFNAME = ../../hal/hal

TARGET_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/ntoskrnl/include -D__ASM__ -DUP

TARGET_CFLAGS = -I../include -I$(PATH_TO_TOP)/ntoskrnl/include -Wall -Werror -DUP

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_INSTALL = no
TARGET_BOOTSTRAP = no

GENERIC_OBJECTS = \
	adapter.o \
	beep.o \
	bus.o \
	dma.o \
	drive.o \
	enum.o \
	fmutex.o \
	halinit.o \
	ipi.o \
	irql.o \
	isa.o \
	kdbg.o \
	mca.o \
	misc.o \
	pci.o \
	portio.o \
	processor.o \
	reboot.o \
	resource.o \
	spinlock.o \
	sysbus.o \
	sysinfo.o \
	time.o \
	timer.o

XBOX_OBJECTS = \
	display_xbox.o \
	font.o \
	halinit_xbox.o \
	part_xbox.o \
	pci_xbox.o

HAL_OBJECTS = $(GENERIC_OBJECTS) $(XBOX_OBJECTS)

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
