#
#  ReactOS regedit
#
#  Makefile
#
#  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
	
PATH_TO_TOP = ..

TARGET = regedit

BASE_CFLAGS = -DGCC -D_WIN32_IE=0x0400

RCFLAGS = -DGCC -D_WIN32_IE=0x0400


OBJS =	about.o \
        framewnd.o \
        childwnd.o \
        listview.o \
        treeview.o \
        trace.o \
        main.o

LIBS = -lgdi32 -luser32 -lkernel32 -lcomctl32

all:	$(TARGET).exe

$(TARGET).res:	$(TARGET).rc

$(TARGET).exe:	$(OBJS) $(TARGET).coff
	$(CC) -Wl,--subsystem,windows -o $(TARGET).exe $(OBJS) $(TARGET).coff $(LIBS)
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


main.h:		resource.h

about.o:	about.c about.h main.h

main.o:		main.c main.h framewnd.h

framewnd.o:	framewnd.c framewnd.h listview.h treeview.h main.h

listview.o:	listview.c listview.h main.h 

treeview.o:	treeview.c treeview.h main.h 

debug.o:	debug.c debug.h main.h


clean:
	- $(RM) $(OBJS)
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

include $(PATH_TO_TOP)/rules.mak
