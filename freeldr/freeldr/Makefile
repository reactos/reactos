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
FLAGS = -Wall -fno-builtin -DDEBUG -O3
#FLAGS = -Wall -fno-builtin

# asmcode.o has to be first in the link line because it contains the startup code
#OBJS = asmcode.a asmcode.o mb.o boot.o freeldr.o stdlib.o fs.a fs.o fat.o \
#	reactos.o tui.o menu.o miscboot.o options.o linux.o multiboot.o arcname.o \
#	mem.o memory.o debug.o parseini.o registry.o import.o
ASM_OBJS = asmcode.o mb.o boot.o mem.o
C_OBJS = freeldr.o stdlib.o fs.a tui.o menu.o miscboot.o options.o linux.o multiboot.o \
		 reactos/reactos.o reactos/registry.o reactos/reghive.o reactos/hwdetect.o
C_OBJS2 = arcname.o memory.o debug.o parseini.o rs232.o portio.o oslist.o

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

freeldr.o:	freeldr.c freeldr.h stdlib.h fs.h reactos/reactos.h tui.h asmcode.h menu.h miscboot.h Makefile
	$(CC) $(FLAGS) -o freeldr.o -c freeldr.c

stdlib.o:	stdlib.c freeldr.h stdlib.h Makefile
	$(CC) $(FLAGS) -o stdlib.o -c stdlib.c

fs.a:	fs.o fat.o Makefile
	$(LD) -r -o fs.a fs.o fat.o

fs.o:	fs.c freeldr.h fs.h stdlib.h tui.h asmcode.h Makefile
	$(CC) $(FLAGS) -o fs.o -c fs.c

fat.o:	fat.c fat.h freeldr.h fs.h stdlib.h tui.h Makefile
	$(CC) $(FLAGS) -o fat.o -c fat.c

reactos/reactos.o:	reactos/reactos.c freeldr.h reactos/reactos.h reactos/registry.h reactos/hwdetect.h stdlib.h fs.h tui.h multiboot.h Makefile
	$(CC) $(FLAGS) -o reactos/reactos.o -c reactos/reactos.c

multiboot.o:	multiboot.c freeldr.h stdlib.h fs.h multiboot.h tui.h Makefile
	$(CC) $(FLAGS) -o multiboot.o -c multiboot.c

mb.o:	mb.S asmcode.h multiboot.h Makefile
	$(CC) $(FLAGS) -o mb.o -c mb.S

tui.o:	tui.c freeldr.h stdlib.h tui.h Makefile
	$(CC) $(FLAGS) -o tui.o -c tui.c

menu.o:	menu.c freeldr.h stdlib.h tui.h menu.h Makefile
	$(CC) $(FLAGS) -o menu.o -c menu.c

boot.o:	boot.S asmcode.h Makefile
	$(CC) $(FLAGS) -o boot.o -c boot.S

miscboot.o:	miscboot.c freeldr.h asmcode.h stdlib.h fs.h tui.h miscboot.h Makefile
	$(CC) $(FLAGS) -o miscboot.o -c miscboot.c

options.o:	options.c freeldr.h stdlib.h tui.h options.h Makefile
	$(CC) $(FLAGS) -o options.o -c options.c

linux.o:	linux.c freeldr.h stdlib.h tui.h linux.h Makefile
	$(CC) $(FLAGS) -o linux.o -c linux.c

arcname.o:	arcname.c freeldr.h arcname.h stdlib.h Makefile
	$(CC) $(FLAGS) -o arcname.o -c arcname.c

mem.o:	mem.S asmcode.h Makefile
	$(CC) $(FLAGS) -o mem.o -c mem.S

memory.o:	memory.c memory.h Makefile
	$(CC) $(FLAGS) -o memory.o -c memory.c

debug.o:	debug.c debug.h Makefile
	$(CC) $(FLAGS) -o debug.o -c debug.c

parseini.o:	parseini.c parseini.h Makefile
	$(CC) $(FLAGS) -o parseini.o -c parseini.c

rs232.o:	rs232.c rs232.h Makefile
	$(CC) $(FLAGS) -o rs232.o -c rs232.c

portio.o:	portio.c portio.h Makefile
	$(CC) $(FLAGS) -o portio.o -c portio.c

oslist.o:	oslist.c oslist.h Makefile
	$(CC) $(FLAGS) -o oslist.o -c oslist.c

reactos/registry.o:	reactos/registry.c freeldr.h memory.h reactos/registry.h stdlib.h Makefile
	$(CC) $(FLAGS) -o reactos/registry.o -c reactos/registry.c

reactos/reghive.o:	reactos/reghive.c freeldr.h reactos/registry.h stdlib.h memory.h Makefile
	$(CC) $(FLAGS) -o reactos/reghive.o -c reactos/reghive.c

reactos/hwdetect.o:	reactos/hwdetect.c freeldr.h reactos/hwdetect.h Makefile
	$(CC) $(FLAGS) -o reactos/hwdetect.o -c reactos/hwdetect.c

clean:
	$(RM) *.o
	$(RM) *.a
	$(RM) *.sys
