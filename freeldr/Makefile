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
	

export CC	= gcc
export LD	= ld
export AR	= ar
export RM	= cmd /C del
export CP	= cmd /C copy
export NASM	= nasm
export MAKE	= make

.PHONY : bootsect freeldr install clean

all: bootsect freeldr install

bootsect:
	$(MAKE) -C bootsect

freeldr: bootsect
	$(MAKE) -C freeldr

install:
	$(MAKE) -C install

clean:
	$(RM) *.bin
