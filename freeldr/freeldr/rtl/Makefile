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

FLAGS = -Wall -fno-builtin -I../ -DDEBUG -O3

OBJS = memory.o print.o stdlib.o string.o

.PHONY : clean

all: rtl.a

rtl.a:	$(OBJS)
	$(LD) -r -o rtl.a $(OBJS)

memory.o:	memory.c ../rtl.h Makefile
	$(CC) $(FLAGS) -o memory.o -c memory.c

print.o:	print.c ../rtl.h Makefile
	$(CC) $(FLAGS) -o print.o -c print.c

stdlib.o:	stdlib.c ../rtl.h Makefile
	$(CC) $(FLAGS) -o stdlib.o -c stdlib.c

string.o:	string.c ../rtl.h Makefile
	$(CC) $(FLAGS) -o string.o -c string.c

clean:
	$(RM) *.o
	$(RM) *.a
