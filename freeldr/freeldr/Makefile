#
#  FreeLoader
#  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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
FLAGS = -Wall -fno-builtin

# asmcode.o has to be first in the link line because it contains the startup code
OBJS = asmcode.a asmcode.o mb.o boot.o freeldr.o stdlib.o fs.a fs.o fs_fat.o \
	reactos.o tui.o menu.o miscboot.o options.o linux.o multiboot.o arcname.o
ASM_OBJS = asmcode.o mb.o boot.o
C_OBJS = freeldr.o stdlib.o fs.a reactos.o tui.o menu.o miscboot.o options.o linux.o \
	multiboot.o arcname.o

all:	freeldr.sys

freeldr.sys:	asmcode.a c_code.a
	$(LD) -N -Ttext=0x8000 --oformat=binary -o freeldr.sys asmcode.a c_code.a

asmcode.a:	$(ASM_OBJS)
	$(LD) -r -o asmcode.a $(ASM_OBJS)

c_code.a:	$(C_OBJS)
	$(LD) -r -o c_code.a $(C_OBJS)

asmcode.o:	asmcode.S asmcode.h Makefile
	$(CC) $(FLAGS) -o asmcode.o -c asmcode.S

freeldr.o:	freeldr.c freeldr.h stdlib.h fs.h reactos.h tui.h asmcode.h menu.h miscboot.h Makefile
	$(CC) $(FLAGS) -o freeldr.o -c freeldr.c

stdlib.o:	stdlib.c freeldr.h stdlib.h Makefile
	$(CC) $(FLAGS) -o stdlib.o -c stdlib.c

fs.a:	fs.o fs_fat.o Makefile
	$(LD) -r -o fs.a fs.o fs_fat.o

fs.o:	fs.c freeldr.h fs.h stdlib.h tui.h asmcode.h Makefile
	$(CC) $(FLAGS) -o fs.o -c fs.c

fs_fat.o:	fs_fat.c freeldr.h fs.h stdlib.h tui.h Makefile
	$(CC) $(FLAGS) -o fs_fat.o -c fs_fat.c

reactos.o:	reactos.c freeldr.h reactos.h stdlib.h fs.h tui.h multiboot.h Makefile
	$(CC) $(FLAGS) -o reactos.o -c reactos.c

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

clean:
	$(RM) *.o
	$(RM) *.a
	$(RM) freeldr.sys
