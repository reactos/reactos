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
DEBUG		=	yes
# Debugging information off (smaller binary)
#DEBUG		=	no

OBJDIR		=	obj
OUTPUT_DIR	=	$(OBJDIR)/$(TARGET)

#############################################
# COMPILER AND LINKER PROGRAMS
#
CC			= gcc
LD			= ld
AR			= ar
NM			= nm
RM			= cmd /C del
CP			= cmd /C copy
MKDIR		= cmd /C md
RMDIR		= cmd /C rd
NASM_CMD	= nasm
OBJCOPY		= objcopy
DEPTOOL		= $(SRCDIR)/../tools/deptool
TOOLSDIR	= $(SRCDIR)/../tools

#-----------------------------------------------------------------------------------------------------
# TEST IF WE ARE IN THE TARGET DIRECTORY
# IF NOT WE WILL CHANGE TO THE TARGET DIRECTORY AND RUN MAKE FROM THERE
#-----------------------------------------------------------------------------------------------------
#ifeq (,$(filter $(CURDIR)/$(OUTPUT_DIR),$(notdir $(CURDIR))))
ifneq ($(CURDIR), $(SRCDIR)/$(OUTPUT_DIR))

.SUFFIXES:

#############################################
# VARIABLE TO CHANGE TO TARGET DIRECTORY AND INVOKE MAKE FROM THERE
#
MAKETARGET	= $(MAKE) --no-print-directory -C $(OUTPUT_DIR) \
			  -f ../../Makefile SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: CHANGE_TO_TARGET
CHANGE_TO_TARGET: $(OBJDIR) $(OBJDIR)/$(TARGET)
	@echo Calculating source file dependencies...
	+@$(MAKETARGET)

$(OBJDIR):
	@echo Creating directory: $(OBJDIR)
	@$(MKDIR) $(OBJDIR)

$(OBJDIR)/$(TARGET): $(OBJDIR)
	@echo Creating directory: $(OBJDIR)\$(TARGET)
	@$(MKDIR) $(OBJDIR)\$(TARGET)

Makefile : ;

% :: CHANGE_TO_TARGET

#############################################

.PHONY : clean
clean:
	@echo Cleaning directory $(OBJDIR)\$(TARGET)
	@-$(RM) /Q $(OBJDIR)\$(TARGET)\*.*
	@echo Removing directory $(OBJDIR)\$(TARGET)
	@-$(RMDIR) $(OBJDIR)\$(TARGET)
	@-$(RMDIR) $(OBJDIR)
	@echo Clean ALL done.

#############################################

#-----------------------------------------------------------------------------------------------------
# END MAGIC TARGET DIRECTORY CHANGE STUFF
#-----------------------------------------------------------------------------------------------------
else

#############################################
# COMPILER COMMAND LINE OPTIONS
#
COMPILER_OPTIONS = -Wall -nostdlib -nostdinc -fno-builtin -O1 -MD

#############################################
# COMPILER DEFINES
#
ifeq ($(DEBUG),yes)
COMPILER_DEFINES = -DDEBUG
else
COMPILER_DEFINES =
endif

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
LINKER_OPTIONS = -N -Ttext=0x8000 -s

#############################################
# LINKERS FLAGS
#
LFLAGS	=	$(LINKER_OPTIONS)

#############################################
# LIST ALL THE OBJECT FILE GROUPS
#
# fathelp.o must come first in the link line because it contains bootsector helper code
# arch.o must come second in the link line because it contains the startup code
ARCH_OBJS	=	fathelp.o	\
				arch.o		\
				i386idt.o	\
				i386trap.o	\
				boot.o		\
				linux.o		\
				mb.o		\
				mem.o		\
				diskint13.o	\
				rtlcode.o	\
				biosvid.o

RTL_OBJS	=	memory.o	\
				print.o		\
				stdlib.o	\
				string.o	\
				list.o

FS_OBJS		=	fs.o		\
				fat.o		\
				iso.o

UI_OBJS		=	tui.o		\
				tuimenu.o	\
				ui.o		\
				gui.o

REACTOS_OBJS=	reactos.o	\
				arcname.o	\
				hwdetect.o	\
				reghive.o	\
				registry.o

COMM_OBJS	=	rs232.o		\
				portio.o

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
				vidmode.o

FREELDR_OBJS=	freeldr.o	\
				miscboot.o	\
				options.o	\
				linuxboot.o	\
				multiboot.o	\
				debug.o		\
				oslist.o	\
				version.o

#############################################
# ALL THE OBJECTS
#
OBJS		=	$(ARCH_OBJS)		\
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
				$(FREELDR_OBJS)

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
				$(SRCDIR)/include

#############################################

all : $(DEPTOOL) freeldr.sys
	@echo Make ALL done.

#############################################

$(DEPTOOL) :
	@$(MAKE) --no-print-directory -C $(TOOLSDIR)

#############################################

freeldr.sys : $(OBJS)
	@echo ===================================================== LINKING $@
#	@$(LD) -N -Ttext=0x8000 --oformat=binary -s -o freeldr.sys $(OBJS)
	@$(LD) $(LFLAGS) -Map freeldr.map -o freeldr.exe $(OBJS)
	@$(OBJCOPY) -O binary freeldr.exe freeldr.sys

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
	@$(NASM_CMD) -o $@ -f coff $<

#############################################

# Include the automagically generated dependencies
-include $(OBJS:%.o=%.d)

#############################################

endif
