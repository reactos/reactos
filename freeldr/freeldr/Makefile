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
	

export CC = gcc
export LD = ld
export AR = ar
export RM = cmd /C del
export CP = cmd /C copy

#FLAGS = -Wall -nostdinc -fno-builtin
FLAGS = -Wall -fno-builtin -I./ -DDEBUG -O3
#FLAGS = -Wall -fno-builtin

# asmcode.o has to be first in the link line because it contains the startup code
#OBJS = asmcode.a asmcode.o mb.o boot.o freeldr.o rtl.o fs.a fs.o fat.o \
#	reactos.o tui.o menu.o miscboot.o options.o linux.o multiboot.o arcname.o \
#	mem.o memory.o debug.o parseini.o registry.o import.o
ASM_OBJS = asmcode.o mb.o boot.o mem.o
C_OBJS = freeldr.o rtl/rtl.a fs/fs.a ui/ui.a miscboot.o options.o linux.o multiboot.o \
		 reactos/reactos.a comm/comm.a disk/disk.a mm/mm.a
C_OBJS2 = debug.o parseini.o oslist.o

.PHONY : clean

all:	freeldr.sys

freeldr.sys:	asmcode.a c_code.a
	$(LD) -N -Ttext=0x8000 --oformat=binary -s -o f.sys asmcode.a c_code.a
	../bootsect/stubit ../bootsect/fatstub.bin f.sys freeldr.sys 

freeldr.exe:	asmcode.a c_code.a
	$(LD) -o freeldr.exe asmcode.a c_code.a

asmcode.a:	$(ASM_OBJS)
	$(LD) -r -o asmcode.a $(ASM_OBJS)

c_code.a:	c_code1.a c_code2.a
	$(LD) -r -o c_code.a c_code1.a c_code2.a

c_code1.a:	$(C_OBJS)
	$(LD) -r -o c_code1.a $(C_OBJS)

c_code2.a:	$(C_OBJS2)
	$(LD) -r -o c_code2.a $(C_OBJS2)

asmcode.o:	asmcode.S asmcode.h Makefile
	$(CC) $(FLAGS) -o asmcode.o -c asmcode.S

mb.o:	mb.S asmcode.h multiboot.h Makefile
	$(CC) $(FLAGS) -o mb.o -c mb.S

boot.o:	boot.S asmcode.h Makefile
	$(CC) $(FLAGS) -o boot.o -c boot.S

mem.o:	mem.S asmcode.h Makefile
	$(CC) $(FLAGS) -o mem.o -c mem.S

freeldr.o:	freeldr.c freeldr.h rtl.h fs.h reactos.h ui.h asmcode.h miscboot.h Makefile
	$(CC) $(FLAGS) -o freeldr.o -c freeldr.c

rtl/rtl.a:	rtl/print.o rtl/stdlib.o rtl/string.o rtl/memory.o Makefile
	$(MAKE) -C rtl

fs/fs.a:	fs/fs.o fs/fat.o Makefile
	$(MAKE) -C fs

ui/ui.a:	ui/tui.o ui/menu.o Makefile
	$(MAKE) -C ui

miscboot.o:	miscboot.c freeldr.h asmcode.h rtl.h fs.h ui.h miscboot.h Makefile
	$(CC) $(FLAGS) -o miscboot.o -c miscboot.c

options.o:	options.c freeldr.h rtl.h ui.h options.h Makefile
	$(CC) $(FLAGS) -o options.o -c options.c

linux.o:	linux.c freeldr.h rtl.h ui.h linux.h Makefile
	$(CC) $(FLAGS) -o linux.o -c linux.c

multiboot.o:	multiboot.c freeldr.h rtl.h fs.h multiboot.h ui.h Makefile
	$(CC) $(FLAGS) -o multiboot.o -c multiboot.c

reactos/reactos.a:	reactos/arcname.o reactos/reactos.o reactos/reghive.o reactos/registry.o reactos/hwdetect.o Makefile
	$(MAKE) -C reactos

comm/comm.a:	comm/rs232.o comm/portio.o Makefile
	$(MAKE) -C comm

disk/disk.a:	disk/disk.o Makefile
	$(MAKE) -C disk

mm/mm.a:	mm/mm.o Makefile
	$(MAKE) -C mm

debug.o:	debug.c debug.h Makefile
	$(CC) $(FLAGS) -o debug.o -c debug.c

parseini.o:	parseini.c parseini.h Makefile
	$(CC) $(FLAGS) -o parseini.o -c parseini.c

oslist.o:	oslist.c oslist.h Makefile
	$(CC) $(FLAGS) -o oslist.o -c oslist.c

clean:
	$(RM) *.o
	$(RM) *.a
	$(RM) *.sys
	$(MAKE) -C reactos clean
	$(MAKE) -C comm clean
	$(MAKE) -C disk clean
	$(MAKE) -C mm clean
	$(MAKE) -C ui clean
	$(MAKE) -C fs clean
	$(MAKE) -C rtl clean
