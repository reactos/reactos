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

PATH_TO_TOP = ../../..

include ../rules.mak

TARGET = i386
ifeq ($(DEBUG),)
# Debugging information on (bigger binary)
#DEBUG = yes
# Debugging information off (smaller binary)
DEBUG =	no
endif

SRCDIR = .

BOOTCD_DIR = $(PATH_TO_TOP)/bootcd

TOOLSDIR = $(SRCDIR)/../tools

DEPTOOL = $(subst /,$(SEP),$(FREELDR_TOOLS_PATH))$(SEP)deptool
TOOLS = $(DEPTOOL)

preall : all

.PHONY: BUILD_TOOLS
BUILD_TOOLS:
	@$(MAKE) --no-print-directory -C $(FREELDR_TOOLS_PATH)

#############################################

.PHONY : clean
clean:
	@-$(RM) $(ALL_OBJS)
	@-$(RM) $(ALL_OBJS:.o=.d)
	@-$(RM) freeldr.exe
	@-$(RM) freeldr.sys
	@-$(RM) freeldr.map
	@-$(RM) setupldr.exe
	@-$(RM) setupldr.sys
	@-$(RM) setupldr.map
	@$(MAKE) --no-print-directory -C $(FREELDR_TOOLS_PATH)
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
boot_loader : freeldr.sys
	$(CP) freeldr.sys $(BOOTCD_DIR)/disk/loader/freeldr.sys
	$(CP) ../freeldr.ini $(BOOTCD_DIR)/disk/loader/freeldr.ini

.PHONY : setup_loader
setup_loader : setupldr.sys
	$(CP) setupldr.sys $(BOOTCD_DIR)/disk/loader/setupldr.sys


COMPILER_OPTIONS = -Wall -Werror -nostdlib -nostdinc -ffreestanding -fno-builtin -fno-inline \
                   -fno-zero-initialized-in-bss -O1 -MD
# FreeLoader does not use any of the standard libraries, includes, or built-in functions

ifeq ($(DEBUG),yes)
COMPILER_DEBUG_DEFINES = -DDEBUG
else
COMPILER_DEBUG_DEFINES =
endif

COMPILER_DEFINES = -D__$(TARGET)__ $(COMPILER_DEBUG_DEFINES)

COMPILER_INCLUDES	=	-I$(SRCDIR)/include -I$(PATH_TO_TOP)/include

CFLAGS = $(COMPILER_OPTIONS) \
         $(COMPILER_DEFINES) \
         $(COMPILER_INCLUDES)

LINKER_OPTIONS = -N -Ttext=0x8000

LFLAGS	=	$(LINKER_OPTIONS)

ifeq ($(HOST),mingw32-linux)
  NASMFLAGS	=	-f win32
else
  NASMFLAGS	=	-f elf
endif

# fathelp.o must come first in the link line because it contains bootsector helper code
# arch.o must come second in the link line because it contains the startup code
ARCH_I386_OBJS = fathelp.o	\
  arch.o \
  i386idt.o \
  i386trap.o \
  i386cpu.o \
  i386pnp.o \
  boot.o \
  linux.o \
  mb.o \
  i386rtl.o	\
  i386vid.o	\
  drvmap.o \
  int386.o \
  i386disk.o \
  portio.o \
  hardware.o \
  hwacpi.o \
  hwapm.o \
  hwcpu.o \
  hwpci.o \
  archmach.o \
  machpc.o \
  machxbox.o \
  pccons.o \
  pcdisk.o \
  pcmem.o \
  pcrtc.o \
  pcvideo.o	\
  xboxcons.o \
  xboxdisk.o \
  xboxfont.o \
  xboxhw.o \
  xboxmem.o	\
  xboxrtc.o	\
  xboxvideo.o	\
  _alloca.o # For Mingw32 builds

ARCH_OBJS	= $(addprefix i386/,$(ARCH_I386_OBJS))

RTL_OBJS = print.o \
  stdlib.o \
  string.o \
  list.o \
  memcmp.o \
  memcpy.o \
  memmove.o \
  memset.o

FS_OBJS =	fs.o \
  fat.o \
  iso.o \
  ext2.o \
  ntfs.o \
  fsrec.o

UI_OBJS =	tui.o \
  tuimenu.o \
  ui.o \
  gui.o

REACTOS_OBJS = arcname.o \
  binhive.o \
  registry.o

COMM_OBJS	=	rs232.o

DISK_OBJS	=	disk.o \
  partition.o

MM_OBJS =	mm.o \
  meminit.o

CACHE_OBJS = cache.o \
  blocklist.o

INIFILE_OBJS = inifile.o \
  ini_init.o \
  parse.o

INFFILE_OBJS = inffile.o

VIDEO_OBJS = video.o \
  fade.o \
  palette.o \
  pixel.o \
  bank.o

# libgcc2.o contains code (__udivdi3, __umoddi3) necessary to do
# 64-bit division on the i386 (and other 32-bit) architectures
# This code was taken from the GCC v3.1 source
MATH_OBJS	=	libgcc2.o

BASE_OBJS	=	freeldr.o \
  debug.o \
  multiboot.o	\
  version.o \
  cmdline.o \
  machine.o

FREELDR_OBJS =	bootmgr.o \
  drivemap.o \
  miscboot.o \
  options.o \
  linuxboot.o	\
  oslist.o \
  custom.o

ROSLDR_OBJS = reactos.o

SETUPLDR_OBJS = setupldr.o

COMMON_OBJS = $(addprefix arch/,$(ARCH_OBJS)) \
  $(addprefix rtl/,$(RTL_OBJS)) \
  $(addprefix fs/,$(FS_OBJS)) \
  $(addprefix ui/,$(UI_OBJS)) \
  $(addprefix reactos/,$(REACTOS_OBJS)) \
  $(addprefix comm/,$(COMM_OBJS)) \
  $(addprefix disk/,$(DISK_OBJS)) \
  $(addprefix mm/,$(MM_OBJS)) \
  $(addprefix cache/,$(CACHE_OBJS)) \
  $(addprefix video/,$(VIDEO_OBJS)) \
  $(addprefix math/,$(MATH_OBJS)) \
  $(BASE_OBJS)

SPECIAL_OBJS = $(addprefix inifile/,$(INIFILE_OBJS)) \
  $(addprefix inffile/,$(INFFILE_OBJS)) \
  $(FREELDR_OBJS) \
  $(addprefix reactos/,$(ROSLDR_OBJS)) \
  $(addprefix reactos/,$(SETUPLDR_OBJS))

F_OBJS = $(COMMON_OBJS) \
  $(addprefix inifile/,$(INIFILE_OBJS)) \
  $(addprefix reactos/,$(ROSLDR_OBJS)) \
  $(FREELDR_OBJS)

S_OBJS = $(COMMON_OBJS) \
  $(addprefix inifile/,$(INIFILE_OBJS)) \
  $(addprefix inffile/,$(INFFILE_OBJS)) \
  $(addprefix reactos/,$(SETUPLDR_OBJS))


ALL_OBJS = $(COMMON_OBJS) \
  $(SPECIAL_OBJS)


all : freeldr.sys setupldr.sys
	@echo Make ALL done.


freeldr.sys : $(ALL_OBJS) $(PATH_TO_TOP)/dk/w32/lib/librossym.a
	@echo ===================================================== LINKING $@
	@$(LD) $(LFLAGS) -o freeldr.exe $(F_OBJS) $(PATH_TO_TOP)/dk/w32/lib/librossym.a
ifeq ($(FULL_MAP),yes)
	@$(OBJDUMP) -d -S freeldr.exe > freeldr.map
else
	@$(NM) --numeric-sort freeldr.exe > freeldr.map
endif
	@$(OBJCOPY) -O binary freeldr.exe freeldr.sys


setupldr.sys : $(ALL_OBJS)
	@echo ===================================================== LINKING $@
	@$(LD) $(LFLAGS) -Map setupldr.map -o setupldr.exe $(S_OBJS)
ifeq ($(FULL_MAP),yes)
	@$(OBJDUMP) -d -S setupldr.exe > setupldr.map
else
	@$(NM) --numeric-sort setupldr.exe > setupldr.map
endif
	@$(OBJCOPY) -O binary setupldr.exe setupldr.sys


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
