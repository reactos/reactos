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
	
include ../rules.mk

OBJS = reactos.o arcname.o hwdetect.o reghive.o registry.o

.PHONY : clean

all: reactos.a

reactos.a:	$(OBJS)
	$(LD) -r -o reactos.a $(OBJS)

reactos.o:	reactos.c ../reactos.h
	$(CC) $(FLAGS) -o reactos.o -c reactos.c

arcname.o:	arcname.c ../reactos.h
	$(CC) $(FLAGS) -o arcname.o -c arcname.c

hwdetect.o:	hwdetect.c ../reactos.h
	$(CC) $(FLAGS) -o hwdetect.o -c hwdetect.c

reghive.o:	reghive.c ../reactos.h
	$(CC) $(FLAGS) -o reghive.o -c reghive.c

registry.o:	registry.c ../reactos.h
	$(CC) $(FLAGS) -o registry.o -c registry.c

clean:
	- $(RM) *.o
	- $(RM) *.a
