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

include rules.mk	

# asmcode.o has to be first in the link line because it contains the startup code
#OBJS = asmcode.a asmcode.o mb.o boot.o freeldr.o rtl.o fs.a fs.o fat.o \
#	reactos.o tui.o menu.o miscboot.o options.o linux.o multiboot.o arcname.o \
#	mem.o memory.o debug.o parseini.o registry.o import.o
#ASM_OBJS = asmcode.o mb.o boot.o mem.o
OBJS = freeldr.o miscboot.o options.o linux.o multiboot.o debug.o oslist.o
LIBS = arch rtl fs ui reactos comm disk mm cache inifile
LIB_FILES = arch/arch.a rtl/rtl.a fs/fs.a ui/ui.a reactos/reactos.a
LIB_FILES2 = comm/comm.a disk/disk.a mm/mm.a cache/cache.a inifile/inifile.a

.PHONY : clean

all:	freeldr.sys

freeldr.sys:	c_code.a end.o
	$(LD) -N -Ttext=0x8000 --oformat=binary -s -o freeldr.sys c_code.a end.o

freeldr.exe:	asmcode.a c_code.a
	$(LD) -o freeldr.exe asmcode.a c_code.a

c_code.a:	$(LIBS) c_code1.a c_code2.a
	$(LD) -r -o c_code.a $(LIB_FILES) c_code1.a c_code2.a

c_code1.a:	$(LIBS)
	$(LD) -r -o c_code1.a $(LIB_FILES2)

c_code2.a:	$(OBJS)
	$(LD) -r -o c_code2.a $(OBJS)

freeldr.o:	freeldr.c freeldr.h rtl.h fs.h reactos.h ui.h arch.h miscboot.h
	$(CC) $(FLAGS) -o freeldr.o -c freeldr.c

miscboot.o:	miscboot.c freeldr.h arch.h rtl.h fs.h ui.h miscboot.h
	$(CC) $(FLAGS) -o miscboot.o -c miscboot.c

options.o:	options.c freeldr.h rtl.h ui.h options.h
	$(CC) $(FLAGS) -o options.o -c options.c

linux.o:	linux.c freeldr.h rtl.h ui.h linux.h
	$(CC) $(FLAGS) -o linux.o -c linux.c

multiboot.o:	multiboot.c freeldr.h rtl.h fs.h multiboot.h ui.h
	$(CC) $(FLAGS) -o multiboot.o -c multiboot.c

debug.o:	debug.c debug.h
	$(CC) $(FLAGS) -o debug.o -c debug.c

oslist.o:	oslist.c oslist.h
	$(CC) $(FLAGS) -o oslist.o -c oslist.c

end.o:	end.S
	$(CC) $(FLAGS) -o end.o -c end.S

arch:
	$(MAKE) -C arch

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

cache:
	$(MAKE) -C cache

inifile:
	$(MAKE) -C inifile

.PHONY : $(LIBS)

clean:
	- $(RM) *.o
	- $(RM) *.a
	- $(RM) *.sys
	$(MAKE) -C arch clean
	$(MAKE) -C reactos clean
	$(MAKE) -C comm clean
	$(MAKE) -C disk clean
	$(MAKE) -C mm clean
	$(MAKE) -C ui clean
	$(MAKE) -C fs clean
	$(MAKE) -C rtl clean
	$(MAKE) -C cache clean
	$(MAKE) -C inifile clean
