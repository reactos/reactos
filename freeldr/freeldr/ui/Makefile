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

OBJS = tui.o menu.o

.PHONY : clean

all: ui.a

ui.a:	$(OBJS)
	$(LD) -r -o ui.a $(OBJS)

tui.o:	tui.c ../ui.h
	$(CC) $(FLAGS) -o tui.o -c tui.c

menu.o:	menu.c ../ui.h
	$(CC) $(FLAGS) -o menu.o -c menu.c

clean:
	- $(RM) *.o
	- $(RM) *.a
