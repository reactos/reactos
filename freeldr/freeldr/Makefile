#
#  FreeLoader
#  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#


#############################################
# CHANGE THESE FOR YOUR OUTPUT
#
TARGET		=	i386
# Debugging information on (bigger binary)
#DEBUG		=	yes
# Debugging information off (smaller binary)
DEBUG		=	no

OBJDIR		=	obj
OUTPUT_DIR	=	$(OBJDIR)/$(TARGET)

BOOTCD_DIR	=	../../bootcd

#############################################
# COMPILER AND LINKER PROGRAMS
#
TOOLSDIR	= $(SRCDIR)/../tools

CC			= gcc
LD			= ld
AR			= ar
NM			= nm
RM			= $(TOOLSDIR)/rdel
CP			= $(TOOLSDIR)/rcopy
MKDIR		= $(TOOLSDIR)/rmkdir
RMDIR		= $(TOOLSDIR)/rrmdir
NASM_CMD	= nasm
OBJCOPY		= objcopy
DEPTOOL		= $(TOOLSDIR)/deptool
HOSTTOOL	= $(TOOLSDIR)/hosttype
TOOLS		= $(DEPTOOL) $(HOSTTOOL)

HOST		= $(shell $(HOSTTOOL))

#-----------------------------------------------------------------------------------------------------
# TEST IF WE ARE IN THE TARGET DIRECTORY
# IF NOT WE WILL CHANGE TO THE TARGET DIRECTORY AND RUN MAKE FROM THERE
#-----------------------------------------------------------------------------------------------------
#ifeq (,$(filter $(CURDIR)/$(OUTPUT_DIR),$(notdir $(CURDIR))))
ifneq ($(CURDIR), $(SRCDIR)/$(OUTPUT_DIR))

SRCDIR		= $(CURDIR)

.SUFFIXES:

#############################################
# VARIABLE TO CHANGE TO TARGET DIRECTORY AND INVOKE MAKE FROM THERE
#
MAKETARGET	= $(MAKE) --no-print-directory -C $(OUTPUT_DIR) \
			  -f ../../Makefile SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: CHANGE_TO_TARGET
CHANGE_TO_TARGET setupldr : BUILD_TOOLS $(OBJDIR) $(OBJDIR)/$(TARGET)
	@echo Calculating source file dependencies...
	+@$(MAKETARGET)

.PHONY: BUILD_TOOLS
BUILD_TOOLS:
	@$(MAKE) --no-print-directory -C $(TOOLSDIR)

$(OBJDIR):
	@echo Creating directory: $(OBJDIR)
	@$(MKDIR) $(OBJDIR)

$(OBJDIR)/$(TARGET): $(OBJDIR)
	@echo Creating directory: $(OBJDIR)/$(TARGET)
	@$(MKDIR) $(OBJDIR)/$(TARGET)


Makefile : ;

% :: CHANGE_TO_TARGET

#############################################

.PHONY : clean
clean:
	@$(MAKE) --no-print-directory -C $(TOOLSDIR)
	@echo Cleaning directory $(OBJDIR)/$(TARGET)
	@-$(RM) $(OBJDIR)/$(TARGET)/*
	@echo Removing directory $(OBJDIR)/$(TARGET)
	@-$(RMDIR) $(OBJDIR)/$(TARGET)
	@-$(RMDIR) $(OBJDIR)
	@echo Clean ALL done.

#############################################

.PHONY : bootcd
bootcd : bootcd_dirs setup_loader boot_loader

.PHONY : bootcd_dirs
bootcd_dirs:
	$(MKDIR) $(BOOTCD_DIR)
	$(MKDIR) $(BOOTCD_DIR)/disk
	$(MKDIR) $(BOOTCD_DIR)/disk/reactos
	$(MKDIR) $(BOOTCD_DIR)/disk/install
	$(MKDIR) $(BOOTCD_DIR)/disk/bootdisk
	$(MKDIR) $(BOOTCD_DIR)/disk/loader

.PHONY : boot_loader
boot_loader : $(OBJDIR)/$(TARGET)/freeldr.sys
	$(CP) $(OBJDIR)/$(TARGET)/freeldr.sys $(BOOTCD_DIR)/disk/loader/freeldr.sys
	$(CP) ../freeldr.ini $(BOOTCD_DIR)/disk/loader/freeldr.ini

.PHONY : setup_loader
setup_loader : $(OBJDIR)/$(TARGET)/setupldr.sys
	$(CP) $(OBJDIR)/$(TARGET)/setupldr.sys $(BOOTCD_DIR)/disk/reactos/setupldr.sys

#############################################

#-----------------------------------------------------------------------------------------------------
# END MAGIC TARGET DIRECTORY CHANGE STUFF
#-----------------------------------------------------------------------------------------------------
else

#############################################
# COMPILER COMMAND LINE OPTIONS
#
COMPILER_OPTIONS = -Wall -nostdlib -nostdinc -ffreestanding -fno-builtin -fno-inline -O1 -MD
# FreeLoader does not use any of the standard libraries, includes, or built-in functions

#############################################
# COMPILER DEFINES
#
ifeq ($(DEBUG),yes)
COMPILER_DEBUG_DEFINES = -DDEBUG
else
COMPILER_DEBUG_DEFINES =
endif

COMPILER_DEFINES = -D__$(TARGET)__ $(COMPILER_DEBUG_DEFINES)

#############################################
# INCLUDE DIRECTORY OPTIONS
#
COMPILER_INCLUDES	=	-I$(SRCDIR)/include

#############################################
# COMPILER FLAGS
#
CFLAGS	=	$(COMPILER_OPTIONS)		\
			$(COMPILER_DEFINES)		\
			$(COMPILER_INCLUDES)

#############################################
# LINKER COMMAND LINE OPTIONS
#
#LINKER_OPTIONS = -N -Ttext=0x8000 --oformat=binary -s
LINKER_OPTIONS = -N -Ttext=0x8000

#############################################
# LINKER FLAGS
#
LFLAGS	=	$(LINKER_OPTIONS)

#############################################
# NASM FLAGS
#
ifeq ($(HOST), dos)
	NASMFLAGS	=	-f coff
else
	ifeq ($(HOST), win32)
		NASMFLAGS	=	-f win32
	else
		NASMFLAGS	=	-f elf
	endif
endif

#############################################
# LIST ALL THE OBJECT FILE GROUPS
#
# fathelp.o must come first in the link line because it contains bootsector helper code
# arch.o must come second in the link line because it contains the startup code
ARCH_OBJS	=	fathelp.o	\
				arch.o		\
				i386idt.o	\
				i386trap.o	\
				i386cpu.o	\
				i386pnp.o	\
				boot.o		\
				linux.o		\
				mb.o		\
				i386mem.o	\
				i386rtl.o	\
				i386vid.o	\
				drvmap.o	\
				int386.o	\
				i386disk.o	\
				portio.o	\
				hardware.o	\
				hwcpu.o		\
				_alloca.o		# For Mingw32 builds


RTL_OBJS	=	print.o		\
				stdlib.o	\
				string.o	\
				list.o		\
				memcmp.o	\
				memcpy.o	\
				memmove.o	\
				memset.o

FS_OBJS		=	fs.o		\
				fat.o		\
				iso.o		\
				ext2.o		\
				fsrec.o

UI_OBJS		=	tui.o		\
				tuimenu.o	\
				ui.o		\
				gui.o

REACTOS_OBJS=	reactos.o	\
				arcname.o	\
				reghive.o	\
				binhive.o	\
				registry.o

COMM_OBJS	=	rs232.o

DISK_OBJS	=	disk.o		\
				geometry.o	\
				partition.o

MM_OBJS		=	mm.o		\
				meminit.o

CACHE_OBJS	=	cache.o		\
				blocklist.o

INIFILE_OBJS=	inifile.o	\
				ini_init.o	\
				parse.o

VIDEO_OBJS	=	video.o		\
				vidmode.o	\
				fade.o		\
				palette.o	\
				pixel.o		\
				bank.o

# libgcc2.o contains code (__udivdi3, __umoddi3) necessary to do
# 64-bit division on the i386 (and other 32-bit) architectures
# This code was taken from the GCC v3.1 source
MATH_OBJS	=	libgcc2.o

BASE_OBJS	=	freeldr.o \
				debug.o \
				drivemap.o \
				multiboot.o	\
				version.o

FREELDR_OBJS=	bootmgr.o \
				miscboot.o \
				options.o \
				linuxboot.o	\
				oslist.o \
				custom.o

SETUPLDR_OBJS=	setupldr.o

COMMON_OBJS	=	$(ARCH_OBJS)		\
				$(RTL_OBJS)			\
				$(FS_OBJS)			\
				$(UI_OBJS)			\
				$(REACTOS_OBJS)		\
				$(COMM_OBJS)		\
				$(DISK_OBJS)		\
				$(MM_OBJS)			\
				$(CACHE_OBJS)		\
				$(INIFILE_OBJS)		\
				$(VIDEO_OBJS)		\
				$(MATH_OBJS)		\
				$(BASE_OBJS)

F_OBJS		=	$(COMMON_OBJS)		\
				$(FREELDR_OBJS)

S_OBJS		=	$(COMMON_OBJS)		\
				$(SETUPLDR_OBJS)

#############################################
# ALL THE OBJECTS
#
ALL_OBJS	=	$(COMMON_OBJS)		\
				$(FREELDR_OBJS)		\
				$(SETUPLDR_OBJS)


#############################################
# SET THE VPATH SO MAKE CAN FIND THE SOURCE FILES
#
VPATH		=	$(SRCDIR)/					\
				$(SRCDIR)/arch/$(TARGET)	\
				$(SRCDIR)/rtl				\
				$(SRCDIR)/fs				\
				$(SRCDIR)/ui				\
				$(SRCDIR)/reactos			\
				$(SRCDIR)/comm				\
				$(SRCDIR)/disk				\
				$(SRCDIR)/mm				\
				$(SRCDIR)/cache				\
				$(SRCDIR)/inifile			\
				$(SRCDIR)/video				\
				$(SRCDIR)/math				\
				$(SRCDIR)/include

#############################################

all : freeldr.sys setupldr.sys
	@echo Make ALL done.

#############################################

freeldr.sys : $(ALL_OBJS)
	@echo ===================================================== LINKING $@
#	@$(LD) -N -Ttext=0x8000 --oformat=binary -s -o freeldr.sys $(F_OBJS)
	@$(LD) $(LFLAGS) -Map freeldr.map -o freeldr.exe $(F_OBJS)
#	@$(CC) -Wl,-Ttext=0x8000,-N,-Map,freeldr.map -o freeldr.exe $(F_OBJS)
	@$(NM) --numeric-sort freeldr.exe > freeldr.sym
	@$(OBJCOPY) -O binary freeldr.exe freeldr.sys

#############################################

setupldr.sys : $(ALL_OBJS)
	@echo ===================================================== LINKING $@
#	@$(LD) -N -Ttext=0x8000 --oformat=binary -s -o setupldr.sys $(S_OBJS)
	@$(LD) $(LFLAGS) -Map setupldr.map -o setupldr.exe $(S_OBJS)
	@$(NM) --numeric-sort setupldr.exe > setupldr.sym
	@$(OBJCOPY) -O binary setupldr.exe setupldr.sys

#############################################

%.o :: %.c
	@echo ===================================================== Compiling $*
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(DEPTOOL) $*.d

%.o :: %.S
	@echo ===================================================== Assembling $*
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(DEPTOOL) $*.d

%.o :: %.asm
	@echo ===================================================== Assembling $*
	@$(NASM_CMD) $(NASMFLAGS) -o $@ $<

#############################################

# Include the automagically generated dependencies
-include $(ALL_OBJS:%.o=%.d)

#############################################

endif
