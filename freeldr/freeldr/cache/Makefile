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

OBJS = cache.o blocklist.o

.PHONY : clean

all: cache.a

cache.a:	$(OBJS)
	$(LD) -r -o cache.a $(OBJS)

cache.o:	cache.c cm.h
	$(CC) $(FLAGS) -o cache.o -c cache.c

blocklist.o:	blocklist.c cm.h
	$(CC) $(FLAGS) -o blocklist.o -c blocklist.c

clean:
	- $(RM) *.o
	- $(RM) *.a
