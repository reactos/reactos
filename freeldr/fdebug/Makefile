#
#  FreeLoader
#  Copyright (C) 1999 - 2003  Brian Palmer  <brianp@sginet.com>
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
	

CC		= gcc
LD		= ld
RM		= cmd /C del

FLAGS = -Wall

OBJS = rs232.o fdebug.o fdebug.res

.PHONY : clean

all: fdebug.exe

fdebug.exe: $(OBJS)
	@echo ===================================================== LINKING fdebug
	$(CC) $(FLAGS) -o fdebug.exe $(OBJS) -lgdi32 -lcomdlg32 -Wl,--subsystem,windows

fdebug.res: fdebug.rc resource.h
	@echo ===================================================== Compiling $*
	windres -o fdebug.res fdebug.rc -O coff

fdebug.o: fdebug.c rs232.h
	@echo ===================================================== Compiling $*
	$(CC) $(FLAGS) -o fdebug.o -c fdebug.c

rs232.o: rs232.c rs232.h
	@echo ===================================================== Compiling $*
	$(CC) $(FLAGS) -o rs232.o -c rs232.c

clean:
	@-$(RM) *.o
	@-$(RM) *.res
	@-$(RM) *.exe
	@echo Clean ALL done.
