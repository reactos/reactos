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
	

CC		= gcc
LD		= ld
RM		= cmd /C del

FLAGS = -Wall

OBJS = install.o volume.o

LIBS = -lkernel32

.PHONY : clean

all: ../bootsect/fat.h ../bootsect/fat32.h install.exe

../bootsect/fat.h ../bootsect/fat32.h :
	@$(MAKE) --no-print-directory -C ../bootsect

install.exe: $(OBJS)
	@echo ===================================================== LINKING install
	$(CC) $(FLAGS) -o install.exe $(OBJS)

install.o: install.c install.h volume.h
	@echo ===================================================== Compiling $*
	$(CC) $(FLAGS) -o install.o -c install.c

volume.o: volume.c volume.h install.h
	@echo ===================================================== Compiling $*
	$(CC) $(FLAGS) -o volume.o -c volume.c

clean:
	@-$(RM) *.o
	@-$(RM) *.exe
	@echo Clean ALL done.
