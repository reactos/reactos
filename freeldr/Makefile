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

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
RM			= rm -f
CP			= cp -f 
MKDIR		= mkdir
SEP      = /
else
RM			= cmd /C del
CP			= copy /Y 
MKDIR		= md
SEP      = \$(EMPTY_VAR)
endif

NASM_CMD	= nasm
NFLAGS = -fwin32 -dwin32
BIN2C		= ..$(SEP)tools$(SEP)bin2c
TOOLSDIR	= ..$(SEP)tools

all:
	$(MAKE) -C tools
	$(MAKE) -C bootsect
	$(MAKE) -C freeldr
	$(MAKE) -C install
	$(MAKE) -C fdebug

clean:
	$(MAKE) -C bootsect clean
	$(MAKE) -C freeldr clean
	$(MAKE) -C install clean
	$(MAKE) -C fdebug clean
	$(MAKE) -C tools clean

.PHONY : clean
