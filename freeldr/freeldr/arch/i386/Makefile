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
	
include ../../rules.mk

OBJS = fathelp.o arch.o boot.o linux.o mb.o mem.o disk.o

.PHONY : clean

all: arch.a

arch.a:	$(OBJS)
	$(LD) -r -o arch.a $(OBJS)
	- $(RM) ..\arch.a
	$(CP) arch.a ..\arch.a

fathelp.o:	fathelp.asm
	$(NASM_CMD) -o fathelp.o -f coff fathelp.asm

arch.o:	arch.S
	$(CC) $(FLAGS) -o arch.o -c arch.S

boot.o:	boot.S
	$(CC) $(FLAGS) -o boot.o -c boot.S

linux.o:	linux.S
	$(CC) $(FLAGS) -o linux.o -c linux.S

mb.o:	mb.S
	$(CC) $(FLAGS) -o mb.o -c mb.S

mem.o:	mem.S
	$(CC) $(FLAGS) -o mem.o -c mem.S

disk.o:	disk.S
	$(CC) $(FLAGS) -o disk.o -c disk.S

clean:
	- $(RM) *.o
	- $(RM) *.a
