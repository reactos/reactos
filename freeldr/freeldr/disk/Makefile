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
	
include ../rules.mk

OBJS = disk.o geometry.o

.PHONY : clean

all: disk.a

disk.a:	$(OBJS)
	$(LD) -r -o disk.a $(OBJS)

disk.o:	disk.c ../disk.h
	$(CC) $(FLAGS) -o disk.o -c disk.c

geometry.o:	geometry.c ../disk.h
	$(CC) $(FLAGS) -o geometry.o -c geometry.c

clean:
	- $(RM) *.o
	- $(RM) *.a
