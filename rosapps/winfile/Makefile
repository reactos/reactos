#
#  ReactOS winfile
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

TARGET = winfile

BASE_CFLAGS = -DGCC -D_WIN32_IE=0x0400

RCFLAGS = -DGCC -D_WIN32_IE=0x0400


OBJS =	about.o \
	debug.o \
        draw.o \
        entries.o \
        main.o \
        mdiclient.o \
        run.o \
        settings.o \
        splitpath.o \
        sort.o \
        utils.o \
	treeview.o \
	listview.o \
	$(TARGET).o

LIBS = -lgdi32 -luser32 -lkernel32 -lcomctl32

all:	$(TARGET).exe

$(TARGET).res:	$(TARGET).rc

$(TARGET).exe:	$(OBJS) $(TARGET).coff
	$(CC) -Wl,--subsystem,windows -o $(TARGET).exe $(OBJS) $(TARGET).coff $(LIBS)
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


about.o:	about.c about.h resource.h

debug.o:	debug.c debug.h

font.o:		font.c font.h

setttings.o:	setttings.c setttings.h resource.h

run.o:		run.c run.h

mdiclient.o:	mdiclient.c mdiclient.h resource.h

$(TARGET).o:	$(TARGET).c $(TARGET).h resource.h


clean:
	- $(RM) $(OBJS)
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

include ../rules.mak
