#
#  FreeLoader
#  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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
	

export CC		= gcc
export LD		= ld
export AR		= ar
export RM		= cmd /C del
export CP		= cmd /C copy
export MAKE		= make

#FLAGS = -Wall -nostdlib -nostdinc -fno-builtin -I./ -DDEBUG -O3
FLAGS = -Wall -nostdlib -nostdinc -fno-builtin -I./ -DDEBUG -O3

# asmcode.o has to be first in the link line because it contains the startup code
#OBJS = asmcode.a asmcode.o mb.o boot.o freeldr.o rtl.o fs.a fs.o fat.o \
#	reactos.o tui.o menu.o miscboot.o options.o linux.o multiboot.o arcname.o \
#	mem.o memory.o debug.o parseini.o registry.o import.o
ASM_OBJS = asmcode.o mb.o boot.o mem.o
OBJS = freeldr.o miscboot.o options.o linux.o multiboot.o debug.o parseini.o oslist.o
LIBS = rtl fs ui reactos comm disk mm
LIB_FILES = rtl/rtl.a fs/fs.a ui/ui.a reactos/reactos.a comm/comm.a disk/disk.a mm/mm.a

.PHONY : clean

all:	freeldr.sys

freeldr.sys:	asmcode.a c_code.a
	$(LD) -N -Ttext=0x8000 --oformat=binary -s -o f.sys asmcode.a c_code.a
	../bootsect/stubit ../bootsect/fatstub.bin f.sys freeldr.sys 

freeldr.exe:	asmcode.a c_code.a
	$(LD) -o freeldr.exe asmcode.a c_code.a

asmcode.a:	$(ASM_OBJS)
	$(LD) -r -o asmcode.a $(ASM_OBJS)

c_code.a:	c_code1.a $(LIBS)
	$(LD) -r -o c_code.a c_code1.a $(LIB_FILES)

c_code1.a:	$(OBJS)
	$(LD) -r -o c_code1.a $(OBJS)

asmcode.o:	asmcode.S asmcode.h
	$(CC) $(FLAGS) -o asmcode.o -c asmcode.S

mb.o:	mb.S asmcode.h multiboot.h
	$(CC) $(FLAGS) -o mb.o -c mb.S

boot.o:	boot.S asmcode.h
	$(CC) $(FLAGS) -o boot.o -c boot.S

mem.o:	mem.S asmcode.h
	$(CC) $(FLAGS) -o mem.o -c mem.S

freeldr.o:	freeldr.c freeldr.h rtl.h fs.h reactos.h ui.h asmcode.h miscboot.h
	$(CC) $(FLAGS) -o freeldr.o -c freeldr.c

miscboot.o:	miscboot.c freeldr.h asmcode.h rtl.h fs.h ui.h miscboot.h
	$(CC) $(FLAGS) -o miscboot.o -c miscboot.c

options.o:	options.c freeldr.h rtl.h ui.h options.h
	$(CC) $(FLAGS) -o options.o -c options.c

linux.o:	linux.c freeldr.h rtl.h ui.h linux.h
	$(CC) $(FLAGS) -o linux.o -c linux.c

multiboot.o:	multiboot.c freeldr.h rtl.h fs.h multiboot.h ui.h
	$(CC) $(FLAGS) -o multiboot.o -c multiboot.c

debug.o:	debug.c debug.h Makefile
	$(CC) $(FLAGS) -o debug.o -c debug.c

parseini.o:	parseini.c parseini.h Makefile
	$(CC) $(FLAGS) -o parseini.o -c parseini.c

oslist.o:	oslist.c oslist.h Makefile
	$(CC) $(FLAGS) -o oslist.o -c oslist.c

rtl:
	$(MAKE) -C rtl

fs:
	$(MAKE) -C fs

ui:
	$(MAKE) -C ui

reactos:
	$(MAKE) -C reactos

comm:
	$(MAKE) -C comm

disk:
	$(MAKE) -C disk

mm:
	$(MAKE) -C mm

.PHONY : $(LIBS)

clean:
	- $(RM) *.o
	- $(RM) *.a
	- $(RM) *.sys
	$(MAKE) -C reactos clean
	$(MAKE) -C comm clean
	$(MAKE) -C disk clean
	$(MAKE) -C mm clean
	$(MAKE) -C ui clean
	$(MAKE) -C fs clean
	$(MAKE) -C rtl clean
