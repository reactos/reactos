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
export NASM_CMD	= nasm

.PHONY : clean

all: fat.bin fat32.bin bin2c.exe

fat.bin: fat.asm bin2c.exe
	$(NASM_CMD) -o fat.bin -f bin fat.asm
	bin2c fat.bin fat.h fat_data


fat32.bin: fat32.asm bin2c.exe
	$(NASM_CMD) -o fat32.bin -f bin fat32.asm
	bin2c fat32.bin fat32.h fat32_data

bin2c.exe: bin2c.c
	$(CC) -o bin2c.exe bin2c.c

clean:
	$(RM) *.bin
	$(RM) *.exe
	$(RM) *.h
